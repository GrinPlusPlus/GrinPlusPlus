#pragma once

#include <Net/Compat.h>
#include <Net/Socket.h>
#include <Net/RPC.h>
#include <Net/SocketAddress.h>
#include <inttypes.h>
#include <string>

// Forward Declarations
struct ProxyCredentials;

class Socks5Connection
{
public:
	bool Connect(const SocketAddress& socksAddress, const std::string& destination, const int port);

	std::unique_ptr<RPC::Response> Send(const RPC::Request& request);

private:
	bool Initialize(const ProxyCredentials* pAuth);
	bool Authenticate(const ProxyCredentials* pAuth);
	bool Connect(const std::string& destination, const int port);

	asio::io_context m_context;
	std::unique_ptr<Socket> m_pSocket;
};