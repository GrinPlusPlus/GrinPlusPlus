#pragma once

#include <Net/SocketAddress.h>

#include <vector>
#include <string>

class DNSSeeder
{
public:
	//
	// Connects to one of the "trusted" DNS seeds, and retrieves a collection of IP addresses to MimbleWimble nodes.
	//
	static std::vector<SocketAddress> GetPeersFromDNS();

private:
	static std::vector<IPAddress> Resolve(const std::string& domainName);
};