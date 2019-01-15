#pragma once

#include <P2P/SocketAddress.h>
#include <Config/Config.h>

#include <vector>
#include <string>

// Forward Declarations
struct in_addr;

//
// This is in charge of communication with MimbleWimble dns seeds.
//
class DNSSeeder
{
public:
	DNSSeeder(const Config& config);

	//
	// Connects to one of the "trusted" DNS seeds, and retrieves a collection of IP addresses to MimbleWimble nodes.
	//
	bool GetPeersFromDNS(std::vector<SocketAddress>& addresses) const;

private:	
	const Config& m_config;

	bool GetAddressesFromDNSSeed(const std::string& dnsSeed, std::vector<SocketAddress>& addresses) const;
	bool InitializeWinsock() const;
	IPAddress ParseIPAddress(const in_addr& ipv4Address) const;
};