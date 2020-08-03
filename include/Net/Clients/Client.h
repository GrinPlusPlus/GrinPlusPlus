#pragma once

#include <Common/Compat.h>
#include <Net/SocketAddress.h>
#include <Common/GrinStr.h>
#include <Common/Logger.h>

#include <asio.hpp>
#include <functional>

//
// This class manages socket timeouts by running the io_context using the timed
// io_context::run_for() member function. Each asynchronous operation is given
// a timeout within which it must complete. The socket operations themselves
// use boost::lambda function objects as completion handlers. For a given
// socket operation, the client object runs the io_context to block thread
// execution until the operation completes or the timeout is reached. If the
// io_context::run_for() function times out, the socket is closed and the
// outstanding asynchronous operation is cancelled.
//
template<class Request, class Response>
class Client {
public:
	Client() : m_socket(m_ioContext) { }

	virtual Response Invoke(const Request& request) = 0;

protected:
	void Connect(const SocketAddress& address, const asio::chrono::steady_clock::duration& timeout)
	{
		if (m_socket.is_open())
		{
			m_socket.close();
		}

		asio::ip::address ipAddress(asio::ip::address_v4::from_string(address.GetIPAddress().Format()));
		asio::ip::tcp::endpoint endpoint(ipAddress, address.GetPortNumber());

		// Start the asynchronous operation itself. The boost::lambda function
		// object is used as a callback and will update the ec variable when the
		// operation completes. The blocking_udp_client.cpp example shows how you
		// can use boost::bind rather than boost::lambda.
		asio::error_code ec;
		m_socket.async_connect(endpoint, std::bind(&Client::handle_connect, std::placeholders::_1, &ec));

		// Run the operation until it completes, or until the timeout.
		run(timeout);

		// Determine whether a connection was successfully established.
		if (ec)
		{
			throw asio::system_error(ec);
		}
	}

	void Connect(const std::string& host, const uint16_t port, const asio::chrono::steady_clock::duration& timeout)
	{
		if (m_socket.is_open())
		{
			m_socket.close();
		}

		asio::io_service ios;

		// Step 3. Creating a query.
		asio::ip::tcp::resolver::query resolver_query(host, std::to_string(port), asio::ip::tcp::resolver::query::numeric_service);

		// Step 4. Creating a resolver.
		asio::ip::tcp::resolver resolver(ios);

		// Used to store information about error that happens
		// during the resolution process.
		std::error_code ec;

		// Step 5.
		asio::ip::tcp::resolver::iterator it = resolver.resolve(resolver_query, ec);

		// Handling errors if any.
		if (ec) {
			// Failed to resolve the DNS name. Breaking execution.
			LOG_ERROR_F("Failed to resolve DNS name. Error: {}", ec.message());
			throw asio::system_error(ec);
		}

		// Start the asynchronous operation itself. The boost::lambda function
		// object is used as a callback and will update the ec variable when the
		// operation completes. The blocking_udp_client.cpp example shows how you
		// can use boost::bind rather than boost::lambda.
		ec.clear();
		m_socket.async_connect(it->endpoint(), std::bind(&Client::handle_connect, std::placeholders::_1, &ec));

		// Run the operation until it completes, or until the timeout.
		run(timeout);

		// Determine whether a connection was successfully established.
		if (ec)
		{
			throw asio::system_error(ec);
		}
	}

	GrinStr ReadLine(const asio::chrono::steady_clock::duration& timeout)
	{
		// Start the asynchronous operation. The boost::lambda function object is
		// used as a callback and will update the ec variable when the operation
		// completes. The blocking_udp_client.cpp example shows how you can use
		// boost::bind rather than boost::lambda.
		std::size_t n;
		asio::error_code ec;
		asio::async_read_until(m_socket,
			asio::dynamic_buffer(m_stringBuffer),
			'\n', std::bind(&Client::handle_receive, std::placeholders::_1, std::placeholders::_2, &ec, &n));

		// Run the operation until it completes, or until the timeout.
		run(timeout);

		// Determine whether the read completed successfully.
		if (ec)
		{
			throw asio::system_error(ec);
		}

		GrinStr line(m_stringBuffer.substr(0, n - 1));
		m_stringBuffer.erase(0, n);
		return line;
	}

	std::vector<uint8_t> Read(const size_t numBytes, const asio::chrono::steady_clock::duration& timeout)
	{
		std::vector<uint8_t> bytes;
		if (m_stringBuffer.size() >= numBytes)
		{
			std::string line(m_stringBuffer.substr(0, numBytes));
			m_stringBuffer.erase(0, numBytes);
			return std::vector<uint8_t>(line.begin(), line.end());
		}

		// Start the asynchronous operation. The boost::lambda function object is
		// used as a callback and will update the ec variable when the operation
		// completes. The blocking_udp_client.cpp example shows how you can use
		// boost::bind rather than boost::lambda.
		std::size_t n;
		asio::error_code ec;
		asio::async_read(m_socket,
			asio::dynamic_buffer(m_stringBuffer),
			asio::transfer_exactly(numBytes - m_stringBuffer.size()),
			std::bind(&Client::handle_receive, std::placeholders::_1, std::placeholders::_2, &ec, &n));

		// Run the operation until it completes, or until the timeout.
		run(timeout);

		// Determine whether the read completed successfully.
		if (ec)
		{
			throw asio::system_error(ec);
		}

		std::string line(m_stringBuffer.substr(0, numBytes));
		m_stringBuffer.erase(0, numBytes);
		return std::vector<uint8_t>(line.begin(), line.end());
	}

	template<class T>
	void Write(const T& data, const asio::chrono::steady_clock::duration& timeout)
	{
		// Start the asynchronous operation. The boost::lambda function object is
		// used as a callback and will update the ec variable when the operation
		// completes. The blocking_udp_client.cpp example shows how you can use
		// boost::bind rather than boost::lambda.
		asio::error_code ec;
		asio::async_write(m_socket, asio::buffer(data),
			std::bind(&Client::handle_connect, std::placeholders::_1, &ec));

		// Run the operation until it completes, or until the timeout.
		run(timeout);

		// Determine whether the read completed successfully.
		if (ec)
		{
			throw asio::system_error(ec);
		}
	}

private:
	void run(const asio::chrono::steady_clock::duration& timeout)
	{
		// Restart the io_context, as it may have been left in the "stopped" state
		// by a previous operation.
		m_ioContext.restart();

		// Block until the asynchronous operation has completed, or timed out. If
		// the pending asynchronous operation is a composed operation, the deadline
		// applies to the entire operation, rather than individual operations on
		// the socket.
		m_ioContext.run_for(timeout);

		// If the asynchronous operation completed successfully then the io_context
		// would have been stopped due to running out of work. If it was not
		// stopped, then the io_context::run_for call must have timed out.
		if (!m_ioContext.stopped()) {
			// Close the socket to cancel the outstanding asynchronous operation.
			m_socket.close();

			// Run the io_context again until the operation completes.
			m_ioContext.run();
		}
	}

	static void handle_connect(const asio::error_code& ec, asio::error_code* out_ec)
	{
		*out_ec = ec;
	}

	static void handle_receive(const asio::error_code& ec, std::size_t length, asio::error_code* out_ec, std::size_t* out_length)
	{

		*out_ec = ec;
		*out_length = length;
	}

	asio::io_context m_ioContext;
	asio::ip::tcp::socket m_socket;
	std::string m_stringBuffer;
};