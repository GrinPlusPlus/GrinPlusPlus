#pragma once

#include <Net/SocketAddress.h>
#include <inttypes.h>
#include <vector>
#include <WinSock2.h>

class Socket
{
public:
	Socket(const SOCKET& socket, const SocketAddress& address, const bool blocking, const unsigned long receiveTimeout, const unsigned long sendTimeout);

	bool CloseSocket();
	bool IsConnected() const;

	inline const SocketAddress& GetSocketAddress() const { return m_address; }
	inline const IPAddress& GetIPAddress() const { return m_address.GetIPAddress(); }
	inline uint16_t GetPort() const { return m_address.GetPortNumber(); }

	bool SetReceiveTimeout(const unsigned long milliseconds);
	inline unsigned long GetReceiveTimeout() const { return m_receiveTimeout; }

	bool SetSendTimeout(const unsigned long milliseconds);
	inline unsigned long GetSendTimeout() const { return m_sendTimeout; }

	bool SetReceiveBufferSize(const int size);

	bool SetBlocking(const bool blocking);
	inline bool IsBlocking() const { return m_blocking; }

	bool Send(const std::vector<unsigned char>& message);

	bool HasReceivedData(const long timeoutMillis) const;
	bool Receive(const size_t numBytes, std::vector<unsigned char>& data) const;

private:
	SOCKET m_socket;

	SocketAddress m_address;
	bool m_blocking;
	unsigned long m_receiveTimeout;
	unsigned long m_sendTimeout;
	int m_receiveBufferSize;
	// TODO: Connection stats
	// TODO: Cache IP address, port, timeouts, blocking, etc.
};