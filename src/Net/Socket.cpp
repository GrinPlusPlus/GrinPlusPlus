#include <Net/Socket.h>
#include <Net/SocketException.h>
#include <Core/Global.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>

static unsigned long DEFAULT_TIMEOUT = 5 * 1000; // 5s

#ifndef _WIN32
#define SOCKET_ERROR -1
#endif

Socket::Socket(const SocketAddress& address)
	: m_address(address),
	m_socketOpen(false),
	m_blocking(true),
	m_receiveBufferSize(0),
	m_receiveTimeout(DEFAULT_TIMEOUT),
	m_sendTimeout(DEFAULT_TIMEOUT)
{

}

Socket::~Socket()
{
	m_pSocket.reset();
	m_pContext.reset();
}

bool Socket::Connect(std::shared_ptr<asio::io_context> pContext)
{
	m_pContext = pContext;
	asio::ip::tcp::endpoint endpoint(asio::ip::address(asio::ip::address_v4::from_string(m_address.GetIPAddress().Format())), m_address.GetPortNumber());

	m_pSocket = std::make_shared<asio::ip::tcp::socket>(*pContext);
	m_pSocket->async_connect(endpoint, [this](const asio::error_code & ec)
		{
			m_errorCode = ec;
			if (!ec)
            {
                asio::socket_base::receive_buffer_size option(32768);
                m_pSocket->set_option(option);
                
				#ifdef _WIN32
				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}

				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}
				#endif

				m_address = SocketAddress(m_address.GetIPAddress(), m_pSocket->remote_endpoint().port());
				m_socketOpen = true;
				return true;
			}
			else
			{
				asio::error_code ignoreError;
				m_pSocket->close(ignoreError);
				return false;
			}
		}
	);

	pContext->run();

	auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(1);
	while (!m_errorCode && !m_socketOpen && std::chrono::system_clock::now() < timeout)
	{
		ThreadUtil::SleepFor(std::chrono::milliseconds(10));
	}

	return m_socketOpen;
}

bool Socket::Accept(std::shared_ptr<asio::io_context> pContext, asio::ip::tcp::acceptor& acceptor, const std::atomic_bool& terminate)
{
	m_pContext = pContext;
	m_pSocket = std::make_shared<asio::ip::tcp::socket>(*pContext);
	acceptor.async_accept(*m_pSocket, [this, pContext](const asio::error_code & ec)
		{
			m_errorCode = ec;
			if (!ec)
			{
				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}

				if (setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)& DEFAULT_TIMEOUT, sizeof(DEFAULT_TIMEOUT)) == SOCKET_ERROR)
				{
					return false;
				}

				const std::string address = m_pSocket->remote_endpoint().address().to_string();
				m_address = SocketAddress(address, m_pSocket->remote_endpoint().port());
				m_socketOpen = true;
				return true;
			}
			else
			{
				asio::error_code ignoreError;
				m_pSocket->close(ignoreError);
				return false;
			}
		}
	);

	while (!m_errorCode && !m_socketOpen)
	{
		if (terminate)
		{
			break;
		}

		pContext->run_one_for(std::chrono::milliseconds(100));
	}

	pContext->reset();

	return m_socketOpen;
}

bool Socket::CloseSocket()
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	m_socketOpen = false;

	asio::error_code error;
	m_pSocket->shutdown(asio::socket_base::shutdown_both, error);
	if (!error)
	{
		m_pSocket->close(error);
	}

	return !error;
}

bool Socket::IsSocketOpen() const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	return m_socketOpen;
}

bool Socket::IsActive() const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	if (m_socketOpen && !m_errorCode)
	{
		return true;
	}

	if (m_errorCode.value() == EAGAIN || m_errorCode.value() == EWOULDBLOCK)
	{
		return true;
	}

	if (m_errorCode)
	{
		LOG_INFO_F("Connection with ({}) not active. Error: {}", m_address, m_errorCode.message());
	}
	
	return false;
}

bool Socket::SetReceiveTimeout(const unsigned long milliseconds)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

#ifdef _WIN32
	const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&milliseconds, sizeof(milliseconds));
#else
	struct timeval timeout;      
	timeout.tv_sec = 0;
	timeout.tv_usec = milliseconds * 1000;
	const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#endif
	if (result == 0)
	{
		m_receiveTimeout = milliseconds;
	}
	else
	{
		return false;
	}

	return true;
}

bool Socket::SetReceiveBufferSize(const int bufferSize)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	const int socketRcvBuff = bufferSize;
	const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_RCVBUF, (const char*)&socketRcvBuff, sizeof(int));
	if (result == 0)
	{
		m_receiveBufferSize = bufferSize;
	}
	else
	{
		return false;
	}

	return true;
}

bool Socket::SetSendTimeout(const unsigned long milliseconds)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

#ifdef _WIN32
	const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)&milliseconds, sizeof(milliseconds));
#else
	struct timeval timeout;      
	timeout.tv_sec = 0;
	timeout.tv_usec = milliseconds * 1000;
	const int result = setsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
#endif

	if (result == 0)
	{
		m_sendTimeout = milliseconds;
		return true;
	}
	else
	{
		return false;
	}
}

bool Socket::SetBlocking(const bool blocking)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	if (m_blocking != blocking)
	{
		#ifdef _WIN32
		unsigned long blockingValue = (blocking ? 0 : 1);
		const int result = ioctlsocket(m_pSocket->native_handle(), FIONBIO, &blockingValue);
		if (result == 0)
		{
			m_blocking = blocking;
		}
		else
		{
			int error = 0;
			int size = sizeof(error);
			getsockopt(m_pSocket->native_handle(), SOL_SOCKET, SO_ERROR, (char*)&error, &size);
			WSASetLastError(error);
			throw SocketException(m_errorCode);
		}
		#else
		m_blocking = blocking;
		#endif
	}

	return m_blocking == blocking;
}

bool Socket::Send(const std::vector<uint8_t>& message, const bool incrementCount)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	if (incrementCount)
	{
		m_rateCounter.AddMessageSent();
	}

	const size_t bytesWritten = asio::write(*m_pSocket, asio::buffer(message.data(), message.size()), m_errorCode);
	if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK)
	{
		throw SocketException(m_errorCode);
	}

	return bytesWritten == message.size();
}

bool Socket::Receive(const size_t numBytes, const bool incrementCount, const ERetrievalMode mode, std::vector<uint8_t>& data)
{
	bool hasReceivedData = HasReceivedData();
	if (mode == BLOCKING)
	{
		std::chrono::time_point timeout = std::chrono::system_clock::now() + std::chrono::seconds(8);
		while (!hasReceivedData)
		{
			if (std::chrono::system_clock::now() >= timeout || !Global::IsRunning())
			{
				return false;
			}

			ThreadUtil::SleepFor(std::chrono::milliseconds(5));
			hasReceivedData = HasReceivedData();
		}
	}

	if (!hasReceivedData) {
		return false;
	}

	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	if (data.size() < numBytes)
	{
		data.resize(numBytes);
	}

	size_t numTries = 0;
	size_t bytesRead = 0;
	while (numTries++ < 3)
	{
		bytesRead += asio::read(*m_pSocket, asio::buffer(data.data() + bytesRead, numBytes - bytesRead), m_errorCode);
		if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK)
		{
			throw SocketException(m_errorCode);
		}

		if (bytesRead == numBytes)
		{
			if (incrementCount)
			{
				m_rateCounter.AddMessageReceived();
			}

			return true;
		}
		else if (m_errorCode.value() == EAGAIN || m_errorCode.value() == EWOULDBLOCK)
		{
			LOG_DEBUG("EAGAIN error returned. Pausing briefly, and then trying again.");
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
	}

	return false;
}

bool Socket::HasReceivedData()
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	const size_t available = m_pSocket->available(m_errorCode);
	if (m_errorCode && m_errorCode.value() != EAGAIN && m_errorCode.value() != EWOULDBLOCK)
	{
		throw SocketException(m_errorCode);
	}
	
	return available >= 11;
}