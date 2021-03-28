#pragma once

#include <Net/Clients/RPC/RPCClient.h>

class HttpConnection
{
public:
	using UPtr = std::unique_ptr<HttpConnection>;

	HttpConnection(const std::string& host, const uint16_t port)
		: m_host(host), m_port(port) { }

	static HttpConnection::UPtr Connect(const std::string& host, const uint16_t port)
	{
		return std::make_unique<HttpConnection>(host, port);
	}
	
	RPC::Response Invoke(const std::string& location, const RPC::Request& request)
	{
		try
		{
			return m_rpcClient.Invoke(m_host, location, m_port, request);
		}
		catch (RPCException&)
		{
			throw;
		}
		catch (std::exception& e)
		{
			throw RPC_EXCEPTION(e.what(), request.GetId());
		}
	}

	RPC::Response Invoke(const std::string& location, const std::string& method, const Json::Value& params)
	{
		return Invoke(location, RPC::Request::BuildRequest(method, params));
	}

	RPC::Response Invoke(const std::string& location, const std::string& method)
	{
		return Invoke(location, RPC::Request::BuildRequest(method));
	}

private:
	std::string m_host;
	uint16_t m_port;
	HttpRpcClient m_rpcClient;
};