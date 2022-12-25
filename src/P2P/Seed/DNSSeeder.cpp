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
			"mainnet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"mainnet.seed.grin.prokapi.com",	// hendi@prokapi.com
			"grinseed.revcore.net",			// yeastplume@gmail.com
			"mainnet-seed.grinnode.live",		// info@grinnode.live
			"mainnet.grin.punksec.de",		// grin@punksec.de
			"grinnode.30-r.com",			// trinitron@30-r.com
		};
	}
	else
	{
		dnsSeeds = {
			"floonet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"floonet.seed.grin.prokapi.com",	// hendi@prokapi.com
			"grintestseed.revcore.net",		// yeastplume@gmail.com
			"testnet.grin.punksec.de",		// grin@punksec.de
			"testnet.grinnode.30-r.com",		// trinitron@30-r.com
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

