#pragma once

#include <Net/Clients/SOCKS/SOCKSClient.h>
#include <Net/Clients/RPC/RPCClient.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorException.h>

class TorConnection
{
public:
	TorConnection(const TorAddress& address, SocketAddress&& proxyAddress)
		: m_address(address), m_socksClient(std::move(proxyAddress)), m_rpcClient(m_socksClient)
	{

	}

	RPC::Response Invoke(const uint16_t portNumber, const RPC::Request& request)
	{
		try
		{
			return m_rpcClient.Invoke(m_address.ToString(), "/v2/foreign", portNumber, request);
		}
		catch (TorException& e)
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
	SOCKSClient m_socksClient;
	HttpRpcClient m_rpcClient;
};