#pragma once

#include <Net/SocketAddress.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorException.h>
#include <memory>

// Forward Declarations
class HttpRpcClient;

class ITorConnection
{
public:
	using Ptr = std::shared_ptr<ITorConnection>;

	virtual RPC::Response Invoke(const RPC::Request& request, const std::string& location) = 0;
};

class TorConnection : public ITorConnection
{
public:
	TorConnection(const TorAddress& address, SocketAddress&& proxyAddress);

	RPC::Response Invoke(const RPC::Request& request, const std::string& location) final;

private:
	TorAddress m_address;
	std::shared_ptr<HttpRpcClient> m_pRpcClient;
};

typedef std::shared_ptr<ITorConnection> TorConnectionPtr;