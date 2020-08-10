#include <Net/Tor/TorConnection.h>

#include <Net/Clients/SOCKS/SOCKSClient.h>
#include <Net/Clients/RPC/RPCClient.h>

TorConnection::TorConnection(const TorAddress& address, SocketAddress&& proxyAddress)
	: m_address(address)
{
	auto pSocksClient = std::shared_ptr<HTTPClient>((HTTPClient*)new SOCKSClient(std::move(proxyAddress)));
	m_pRpcClient = std::make_shared<HttpRpcClient>(pSocksClient);
}

RPC::Response TorConnection::Invoke(const RPC::Request& request, const std::string& location)
{
	try
	{
		return m_pRpcClient->Invoke(m_address.ToString(), location, 80, request);
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