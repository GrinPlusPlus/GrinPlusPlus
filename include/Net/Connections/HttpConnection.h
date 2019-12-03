#pragma once

#include <Net/Clients/RPC/RPCClient.h>

class HttpConnection
{
public:
	static std::shared_ptr<HttpConnection> Connect(const std::string& address)
	{
		// TODO: Finish this
	}
	
	RPC::Response Invoke(const RPC::Request& request)
	{
		try
		{
			return m_rpcClient.Invoke(m_host, m_location + "/v2/foreign", m_port, request);
		}
		catch (std::exception& e)
		{
			throw RPC_EXCEPTION(e.what());
		}
	}

private:
	HttpConnection(const std::string& host, const uint16_t port, const std::string& location)
		: m_host(host), m_port(port), m_location(location)
	{

	}

	std::string m_host;
	uint16_t m_port;
	std::string m_location;
	HttpRpcClient m_rpcClient;
};