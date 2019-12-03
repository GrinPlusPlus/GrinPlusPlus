#pragma once

#include <Net/Clients/SOCKS/SOCKSClient.h>
#include <Net/Clients/RPC/RPCClient.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorException.h>
#include <memory>

class TorConnection
{
public:
	TorConnection(const TorAddress& address, SocketAddress&& proxyAddress)
		: m_address(address),
		m_rpcClient(std::shared_ptr<HTTPClient>((HTTPClient*)new SOCKSClient(std::move(proxyAddress))))
	{

	}

	RPC::Response Invoke(const RPC::Request& request, const std::string& location)
	{
		try
		{
			return m_rpcClient.Invoke(m_address.ToString(), location, 80, request);
		}
		catch (TorException&)
		{
			throw;
		}
		catch (std::exception& e)
		{
			throw TOR_EXCEPTION(e.what());
		}
	}

private:
	TorAddress m_address;
	HttpRpcClient m_rpcClient;
};

typedef std::shared_ptr<TorConnection> TorConnectionPtr;