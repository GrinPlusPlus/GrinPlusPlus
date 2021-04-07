#include <Net/Socket.h>
#include <Net/SocketException.h>
#include <Core/Global.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>

static unsigned long DEFAULT_TIMEOUT = 2 * 1000;

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
    m_receiveBufferSize(0),
    m_receiveTimeout(DEFAULT_TIMEOUT),
    m_sendTimeout(DEFAULT_TIMEOUT)
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
    //std::shared_lock<std::shared_mutex> readLock(m_mutex);

    if (m_socketOpen && !m_errorCode) {
        return true;
    }

    if (m_errorCode.value() == EAGAIN || m_errorCode.value() == EWOULDBLOCK) {
        return true;
    }

    if (m_errorCode) {
        LOG_INFO_F("Connection with {} not active. Error: {}", m_address, m_errorCode.message());
    } else {
        LOG_INFO_F("Connection with {} not active.", m_address);
    }

    return false;
}

bool Socket::SetDefaultOptions()
{
    asio::socket_base::receive_buffer_size option(32768);
    m_pSocket->set_option(option);

#ifdef _WIN32
    if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR) {
        return false;
    }

    if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)&DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR) {
        return false;
    }
#endif

    return true;
}

bool Socket::SetReceiveTimeout(const unsigned long milliseconds)
{
    //std::unique_lock<std::shared_mutex> writeLock(m_mutex);

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

bool Socket::SetReceiveBufferSize(const int bufferSize)
{
    //std::unique_lock<std::shared_mutex> writeLock(m_mutex);

    const int socketRcvBuff = bufferSize;
    const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVBUF, (const char*)&socketRcvBuff, sizeof(int));
    if (result == 0) {
        m_receiveBufferSize = bufferSize;
    } else {
        return false;
    }

    return true;
}

bool Socket::SetSendTimeout(const unsigned long milliseconds)
{
    //std::unique_lock<std::shared_mutex> writeLock(m_mutex);

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
    //std::unique_lock<std::shared_mutex> writeLock(m_mutex);

    if (m_blocking != blocking) {
#ifdef _WIN32
        unsigned long blockingValue = (blocking ? 0 : 1);
        const int result = ioctlsocket(m_pSocket->native_handle(), FIONBIO, &blockingValue);
        if (result == 0) {
            m_blocking = blocking;
        } else {
            int error = 0;
            int size = sizeof(error);
            getsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_ERROR, (char*)&error, &size);
            WSASetLastError(error);
            ThrowSocketException(m_errorCode);
        }
#else
        m_blocking = blocking;
#endif
    }

    return m_blocking == blocking;
}

bool Socket::SendSync(const std::vector<uint8_t>& message, const bool incrementCount)
{
    std::unique_lock<std::mutex> writeQueueLock(m_writeQueueMutex);

    if (incrementCount) {
        m_rateCounter.AddMessageSent();
    }

    const size_t bytesWritten = asio::write(*m_pSocket, asio::buffer(message.data(), message.size()), m_errorCode);
    if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK) {
        ThrowSocketException(m_errorCode);
    }

    return bytesWritten == message.size();
}

void Socket::SendAsync(const std::vector<uint8_t>& message)
{
    bool first_in_queue = m_writeQueue.empty();
    m_writeQueue.push(message);

    if (!first_in_queue) {
        // There is already an async_write in process.
        // It will send this message when it completes.
        return;
    }

    size_t message_size = message.size();

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
    std::unique_lock<std::mutex> writeQueueLock(m_writeQueueMutex);

    m_rateCounter.AddMessageSent();
    if (!m_writeQueue.empty()) {
        m_writeQueue.pop();
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

std::vector<uint8_t> Socket::ReceiveSync(const size_t num_bytes, const bool incrementCount)
{
    std::chrono::time_point timeout = std::chrono::system_clock::now() + std::chrono::seconds(8);
    while (!HasReceivedData()) {
        if (std::chrono::system_clock::now() >= timeout || !Global::IsRunning()) {
            return {};
        }

        ThreadUtil::SleepFor(std::chrono::milliseconds(5));
    }

    //std::unique_lock<std::shared_mutex> writeLock(m_mutex);

    std::vector<uint8_t> bytes(num_bytes);

    size_t numTries = 0;
    size_t bytesRead = 0;
    while (numTries++ < 3) {
        bytesRead += asio::read(*m_pSocket, asio::buffer(bytes.data() + bytesRead, num_bytes - bytesRead), m_errorCode);
        if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK) {
            ThrowSocketException(m_errorCode);
        }

        if (bytesRead == num_bytes) {
            if (incrementCount) {
                m_rateCounter.AddMessageReceived();
            }

            return bytes;
        } else if (m_errorCode.value() == EAGAIN || m_errorCode.value() == EWOULDBLOCK) {
            LOG_DEBUG("EAGAIN error returned. Pausing briefly, and then trying again.");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    return {};
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