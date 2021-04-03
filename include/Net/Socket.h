#pragma once

#include <Common/Compat.h>
#include <Core/Traits/Printable.h>
#include <Net/RateCounter.h>
#include <Net/SocketAddress.h>

#include <inttypes.h>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>

#include <asio.hpp>

class Socket : public Traits::IPrintable
{
public:
	Socket(
		SocketAddress socket_address,
		const std::shared_ptr<asio::io_context>& pContext,
		const std::shared_ptr<asio::ip::tcp::socket>& pSocket
	);
	virtual ~Socket();

	enum ERetrievalMode
	{
		BLOCKING,
		NON_BLOCKING
	};

	bool SetDefaultOptions();
	bool CloseSocket();
	bool IsActive() const;

	std::string Format() const final { return m_address.Format(); }

	const std::shared_ptr<asio::ip::tcp::socket>& GetAsioSocket() const noexcept { return m_pSocket; }
	const SocketAddress& GetSocketAddress() const { return m_address; }
	asio::ip::tcp::endpoint GetEndpoint() const { return m_address.GetEndpoint(); }
	const IPAddress& GetIPAddress() const { return m_address.GetIPAddress(); }
	uint16_t GetPort() const { return m_address.GetPortNumber(); }
	RateCounter& GetRateCounter() { return m_rateCounter; }

	bool SetReceiveTimeout(const unsigned long milliseconds);
	unsigned long GetReceiveTimeout() const { return m_receiveTimeout; }

	bool SetSendTimeout(const unsigned long milliseconds);
	unsigned long GetSendTimeout() const { return m_sendTimeout; }

	bool SetReceiveBufferSize(const int size);
	int GetReceiveBufferSize() const { return m_receiveBufferSize; }

	bool SetBlocking(const bool blocking);
	bool IsBlocking() const { return m_blocking; }

	bool IsOpen() const {
		std::shared_lock<std::shared_mutex> read_lock(m_mutex);
		return m_socketOpen;
	}

	void SetOpen(bool open) {
		std::unique_lock<std::shared_mutex> write_lock(m_mutex);
		m_socketOpen = open;
	}

	bool IsConnectFailed() const { return m_failed; }
	void SetConnectFailed(bool failed) { m_failed = failed; }

	bool Send(const std::vector<uint8_t>& message, const bool incrementCount);
	void SendAsync(const std::vector<uint8_t>& message, const bool incrementCount);

	bool Receive(
		const size_t numBytes,
		const bool incrementCount,
		const ERetrievalMode mode,
		std::vector<uint8_t>& data
	);

private:
	bool HasReceivedData(const size_t bytes_needed);
	void ThrowSocketException(const asio::error_code& ec);

	std::shared_ptr<asio::ip::tcp::socket> m_pSocket;
	std::shared_ptr<asio::io_context> m_pContext;

	SocketAddress m_address;
	bool m_blocking;
	unsigned long m_receiveTimeout;
	unsigned long m_sendTimeout;
	int m_receiveBufferSize;
	RateCounter m_rateCounter;

	mutable std::shared_mutex m_mutex;
	asio::error_code m_errorCode;
	std::atomic_bool m_socketOpen;
	std::atomic_bool m_failed;
};

typedef std::shared_ptr<Socket> SocketPtr;