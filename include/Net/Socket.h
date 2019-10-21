#pragma once

#include <Net/RateCounter.h>
#include <Net/SocketAddress.h>

#include <inttypes.h>
#include <vector>
#include <memory>
#include <atomic>
#include <asio.hpp>

class Socket
{
public:
	Socket(const SocketAddress& address);

	bool Connect(asio::io_context& context);
	bool Accept(asio::io_context& context, asio::ip::tcp::acceptor& acceptor, const std::atomic_bool& terminate);

	bool CloseSocket();
	bool IsSocketOpen() const;
	bool IsActive() const;

	inline const SocketAddress& GetSocketAddress() const { return m_address; }
	inline const IPAddress& GetIPAddress() const { return m_address.GetIPAddress(); }
	inline uint16_t GetPort() const { return m_address.GetPortNumber(); }
	inline RateCounter& GetRateCounter() { return m_rateCounter; }

	bool SetReceiveTimeout(const unsigned long milliseconds);
	inline unsigned long GetReceiveTimeout() const { return m_receiveTimeout; }

	bool SetSendTimeout(const unsigned long milliseconds);
	inline unsigned long GetSendTimeout() const { return m_sendTimeout; }

	bool SetReceiveBufferSize(const int size);
	inline int GetReceiveBufferSize() const { return m_receiveBufferSize; }

	bool SetBlocking(const bool blocking);
	inline bool IsBlocking() const { return m_blocking; }

	bool Send(const std::vector<unsigned char>& message, const bool incrementCount);

	bool HasReceivedData();
	bool Receive(const size_t numBytes, const bool incrementCount, std::vector<unsigned char>& data);

private:
	std::shared_ptr<asio::ip::tcp::socket> m_pSocket;
	asio::error_code m_errorCode;

	bool m_socketOpen;
	SocketAddress m_address;
	bool m_blocking;
	unsigned long m_receiveTimeout;
	unsigned long m_sendTimeout;
	int m_receiveBufferSize;
	RateCounter m_rateCounter;
};