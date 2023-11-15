#include <Net/Socket.h>
#include <Net/SocketException.h>
#include <Core/Global.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>

static const unsigned long DEFAULT_TIMEOUT = 10 * 1000;
static const unsigned long RECEIVING_TIMEOUT = 1000 * 10;
static const unsigned long SENDING_TIMEOUT = 1000 * 10;

#ifndef _WIN32
#define SOCKET_ERROR -1
#endif

Socket::Socket(
    SocketAddress socket_address,
    const std::shared_ptr<asio::io_context>& pContext,
    const std::shared_ptr<asio::ip::tcp::socket>& pSocket)
    : m_pSocket(pSocket),
    m_pContext(pContext),
    m_address(std::move(socket_address)),
    m_socketOpen(false),
    m_failed(false),
    m_blocking(true),
    m_receiveBufferSize(8192),
    m_sendBufferSize(4096),
    m_receiveTimeout(DEFAULT_TIMEOUT),
    m_sendTimeout(DEFAULT_TIMEOUT),
    m_delayed(false),
    m_keptAlive(true)
{
    
}

Socket::~Socket()
{
    CloseSocket();

    std::lock(m_socketMutex, m_writeQueueMutex);
    m_pSocket.reset();
    m_pContext.reset();
}

bool Socket::CloseSocket()
{
    std::unique_lock<std::shared_mutex> socketLock(m_socketMutex);
    if (!m_socketOpen) {
        return true;
    }

    m_socketOpen = false;

    asio::error_code error;
    m_pSocket->shutdown(asio::socket_base::shutdown_both, error);
    m_pSocket->close(error);
    
    return !error;
}

bool Socket::IsActive() const
{
    if (m_errorCode.value() != EAGAIN &&
        m_errorCode.value() != EWOULDBLOCK) 
    {
        LOG_INFO_F("{} is not active. Error: {}", m_address, m_errorCode.message());
        return false;
    }

    return true;
}

void Socket::SetDefaults()
{
    SetBlocking(true);
    SetKeepAliveOption(true);
    SetSendTimeout(m_sendTimeout);
    SetReceiveTimeout(m_receiveTimeout);
    SetSendBufferSize(m_sendBufferSize);
    SetReceiveBufferSize(m_receiveBufferSize);
}

bool Socket::SetReceiveTimeout(const unsigned long milliseconds)
{
#ifdef _WIN32
    const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&milliseconds, sizeof(milliseconds));
#else
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = milliseconds * 1000;
    const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#endif
    if (result == 0) {
        m_receiveTimeout = milliseconds;
    } else {
        return false;
    }

    return true;
}

bool Socket::SetReceiveBufferSize(const size_t bufferSize)
{
    const size_t socketRcvBuff = bufferSize;
    const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVBUF, (const char*)&socketRcvBuff, sizeof(int));
    if (result == 0) {
        m_receiveBufferSize = bufferSize;
    } else {
        return false;
    }

    return true;
}

bool Socket::SetSendBufferSize(const size_t bufferSize)
{
    const size_t socketSndBuff = bufferSize;
    const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDBUF, (const char*)&socketSndBuff, sizeof(int));
    if (result == 0) {
        m_sendBufferSize = bufferSize;
    }
    else {
        return false;
    }

    return true;
}

bool Socket::SetSendTimeout(const unsigned long milliseconds)
{
#ifdef _WIN32
    const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)&milliseconds, sizeof(milliseconds));
#else
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = milliseconds * 1000;
    const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
#endif

    if (result == 0) {
        m_sendTimeout = milliseconds;
        return true;
    } else {
        return false;
    }
}

bool Socket::SetBlocking(const bool blocking)
{
    m_pSocket->non_blocking(!blocking);
    return m_blocking == !blocking;
}

bool Socket::SetDelayOption(const bool delayed)
{
    asio::ip::tcp::no_delay no_delay_option(delayed);
    m_pSocket->set_option(no_delay_option);
    
    m_delayed = delayed;
    
    return m_delayed == delayed;
}

bool Socket::SetKeepAliveOption(const bool keepAlive)
{
    asio::socket_base::keep_alive keep(keepAlive);
    m_pSocket->set_option(keep);

    m_keptAlive = keepAlive;

    return m_keptAlive == keepAlive;
}

bool Socket::IsKeepAliveEnabled()
{
    return m_keptAlive;
}

bool Socket::SendSync(const std::vector<uint8_t>& message, const bool incrementCount)
{
    const size_t bytesWritten = asio::write(*m_pSocket, asio::buffer(message), asio::transfer_exactly(message.size()), m_errorCode);
    
    if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK) 
    {
        ThrowSocketException(m_errorCode);
    }

    if (incrementCount)
    {
        m_rateCounter.AddMessageSent();
    }

    return bytesWritten == message.size();
}

void Socket::SendAsync(const std::vector<uint8_t>& message)
{
    bool first_in_queue = m_writeQueue.empty();
    m_writeQueue.push_back(message);

    if (!first_in_queue) {
        // There is already an async_write in process.
        // It will send this message when it completes.
        return;
    }

    std::shared_lock<std::shared_mutex> socketLock(m_socketMutex);
    if (m_socketOpen) {
        asio::async_write(
            *m_pSocket,
            asio::buffer(message.data(), message.size()),
            std::bind(&Socket::HandleSent, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
}

void Socket::HandleSent(const asio::error_code& ec, size_t)
{
    m_rateCounter.AddMessageSent();

    if (!m_writeQueue.empty()) {
        m_writeQueue.pop_front();
    }

    if (ec) {
        LOG_INFO_F("Failed to send message to {}: {}", *this, ec.message());
    } else if (!m_writeQueue.empty()) {
        std::vector<uint8_t> message = m_writeQueue.front();

        std::shared_lock<std::shared_mutex> socketLock(m_socketMutex);
        if (m_socketOpen) {
            asio::async_write(
                *m_pSocket,
                asio::buffer(message.data(), message.size()),
                std::bind(&Socket::HandleSent, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
            );
        }
    }
}

std::vector<uint8_t> Socket::ReceiveSync(const size_t bufferSize, const bool incrementCount)
{
    std::vector<uint8_t> bytes(bufferSize);
    size_t recieved = asio::read(*m_pSocket, asio::buffer(bytes), asio::transfer_exactly(bufferSize), m_errorCode);
    if (m_errorCode) { ThrowSocketException(m_errorCode); }

    if (recieved == bufferSize && incrementCount)
    {
        m_rateCounter.AddMessageReceived();
    }
    return bytes;

    /*size_t retries = 0;
    size_t bytesRead = 0;
    do
    {
        if (HasReceivedData())
        {
            bytesRead += asio::read(*m_pSocket, asio::buffer(bytes.data() + bytesRead, num_bytes - bytesRead), m_errorCode);
            if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK) { ThrowSocketException(m_errorCode); }
        }
        retries++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (bytesRead != num_bytes || retries >= 50);
    
    if (bytesRead != num_bytes)  
    { 
        LOG_ERROR_F("Expected to read {}b but instead, {}b were recieved from {}", num_bytes, bytesRead, m_address.Format());
        return bytes;
    }*/
}

bool Socket::HasReceivedData()
{
    const size_t available = m_pSocket->available(m_errorCode);
    if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK) {
        ThrowSocketException(m_errorCode);
    }

    return available > 0;
}

void Socket::ThrowSocketException(const asio::error_code& ec)
{
    std::string error_message = "Socket error occurred";
    if (ec) {
        error_message = ec.message();
    }

#ifdef _WIN32
    const int lastError = WSAGetLastError();

    TCHAR* s = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&s, 0, NULL);

    error_message = StringUtil::ToUTF8(s);
    LocalFree(s);
#endif

    throw SocketException(ec, error_message);
}