#pragma once

#include <Net/Socks5.h>
#include <Net/RPC.h>
#include <Net/Tor/TorAddress.h>

class TorConnection
{
public:
	TorConnection(const TorAddress& address, Socks5Connection&& connection);

	bool CheckVersion();
	RPC::Response Send(const RPC::Request& request);

private:
	TorAddress m_address;
	Socks5Connection m_socksConnection;
};