#include "DNSSeeder.h"

#include <Common/Logger.h>
#include <Core/Config.h>
#include <Core/Global.h>
#include <asio.hpp>

std::vector<SocketAddress> DNSSeeder::GetPeersFromDNS()
{
	std::vector<SocketAddress> addresses;

	std::vector<std::string> dnsSeeds;
	if (Global::IsMainnet())
	{
		dnsSeeds = {
			"mainnet.seed.grin.prokapi.com",	// hendi@prokapi.com
			"mainnet-seed.grinnode.live"	    // info@grinnode.live
			"grinseed.yeastplume.org",			// yeastplume@protonmail.com
			"mainnet.seed.713.mw",				// jasper@713.mw
			"mainnet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"mainnet.seed.grin.icu",			// gary.peverell@protonmail.com
		};
	}
	else
	{
		dnsSeeds = {
			"floonet.seed.713.mw",				// jasper@713.mw
			"floonet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"floonet.seed.grin.prokapi.com",	// hendi@prokapi.com
			"floonet.seed.grin.icu",			// gary.peverell@protonmail.com
		};
	}

	for (auto seed : dnsSeeds)
	{
		LOG_TRACE_F("Checking seed: {}", seed);
		const std::vector<IPAddress> ipAddresses = IPAddress::Resolve(seed);
		for (const IPAddress ipAddress : ipAddresses)
		{
			LOG_TRACE_F("IP Address: {}", ipAddress);
			addresses.emplace_back(SocketAddress(ipAddress, Global::GetConfig().GetP2PPort()));
		}
	}

	return addresses;
}

