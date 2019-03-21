#include "DNSSeeder.h"

#include <Net/DNS.h>

DNSSeeder::DNSSeeder(const Config& config)
	: m_config(config)
{

}

std::vector<SocketAddress> DNSSeeder::GetPeersFromDNS() const
{
	std::vector<SocketAddress> addresses;

	std::vector<std::string> dnsSeeds;
	if (m_config.GetEnvironment().IsMainnet())
	{
		dnsSeeds = {
			"mainnet.seed.grin-tech.org",		// igno.peverell@protonmail.com
			"mainnet.seed.grin.icu",			// gary.peverell@protonmail.com
			"mainnet.seed.713.mw",				// jasper@713.mw
			"mainnet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"mainnet.seed.grin.prokapi.com",	// hendi@prokapi.com
			"grinseed.yeastplume.org",			// yeastplume@protonmail.com
		};
	}
	else
	{
		dnsSeeds = {
			"floonet.seed.grin-tech.org",		// igno.peverell@protonmail.com
			"floonet.seed.grin.icu",			// gary.peverell@protonmail.com
			"floonet.seed.713.mw",				// jasper@713.mw
			"floonet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"floonet.seed.grin.prokapi.com",	// hendi@prokapi.com
		};
	}

	for (auto seed : dnsSeeds)
	{
		const std::vector<IPAddress> ipAddresses = DNS::Resolve(seed);
		for (const IPAddress ipAddress : ipAddresses)
		{
			addresses.emplace_back(SocketAddress(ipAddress, m_config.GetEnvironment().GetP2PPort()));
		}
	}

	return addresses;
}