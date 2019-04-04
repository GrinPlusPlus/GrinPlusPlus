#include <Net/Socket.h>
#include <Net/SocketException.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

Socket::Socket(const SOCKET& socket, const SocketAddress& address, const bool blocking, const unsigned long receiveTimeout, const unsigned long sendTimeout)
	: m_socket(socket), m_address(address), m_blocking(blocking), m_receiveTimeout(receiveTimeout), m_sendTimeout(sendTimeout), m_receiveBufferSize(0)
{

}

bool Socket::CloseSocket()
{
	return closesocket(m_socket) == 0;
}

bool Socket::IsConnected() const
{
	fd_set readFDS;
	readFDS.fd_count = 1;
	readFDS.fd_array[0] = m_socket;

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000; // 1 ms

	const int result = select(0, &readFDS, nullptr, nullptr, &timeout);

	return result != SOCKET_ERROR;
}

bool Socket::SetReceiveTimeout(const unsigned long milliseconds)
{
	if (m_receiveTimeout != milliseconds)
	{
		const int result = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&milliseconds, sizeof(milliseconds));
		if (result == 0)
		{
			m_receiveTimeout = milliseconds;
		}
	}

	return m_receiveTimeout == milliseconds;
}

bool Socket::SetReceiveBufferSize(const int bufferSize)
{
	if (m_receiveBufferSize != bufferSize)
	{
		const int socketRcvBuff = bufferSize;
		const int result = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&socketRcvBuff, sizeof(int));
		if (result == 0)
		{
			m_receiveBufferSize = bufferSize;
		}
	}

	return m_receiveBufferSize == bufferSize;
}

bool Socket::SetSendTimeout(const unsigned long milliseconds)
{
	if (m_sendTimeout != milliseconds)
	{
		const int result = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&milliseconds, sizeof(milliseconds));
		if (result == 0)
		{
			m_sendTimeout = milliseconds;
		}
	}

	return m_sendTimeout == milliseconds;
}

bool Socket::SetBlocking(const bool blocking)
{
	if (m_blocking != blocking)
	{
		unsigned long blockingValue = (blocking ? 0 : 1);
		const int result = ioctlsocket(m_socket, FIONBIO, &blockingValue);
		if (result == 0)
		{
			m_blocking = blocking;
		}
	}

	return m_blocking == blocking;
}

bool Socket::Send(const std::vector<unsigned char>& message)
{
	size_t totalSent = 0;
	while (totalSent < message.size())
	{
		const int newBytesSent = send(m_socket, (const char*)&message[totalSent], (int)(message.size() - totalSent), 0);
		if (newBytesSent <= 0)
		{
			return false;
		}

		totalSent += newBytesSent;
	}

	return true;
}

bool Socket::Receive(const size_t numBytes, std::vector<unsigned char>& data) const
{
	if (data.size() < numBytes)
	{
		data = std::vector<unsigned char>(numBytes);
	}

	size_t totalReceived = 0;
	while (totalReceived < numBytes)
	{
		const int newBytesReceived = recv(m_socket, (char*)&data[totalReceived], (int)(numBytes - totalReceived), 0);
		if (newBytesReceived <= 0)
		{
			return false;
		}

		totalReceived += newBytesReceived;
	}

	return true;
}

bool Socket::HasReceivedData(const long timeoutMillis) const
{
	fd_set readFDS;
	readFDS.fd_count = 1;
	readFDS.fd_array[0] = m_socket;

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = timeoutMillis * 1000;

	const int result = select(0, &readFDS, nullptr, nullptr, &timeout);
	if (result > 0)
	{
		return true;
	}
	else if (result < 0)
	{
		throw SocketException();
	}

	return false;
}