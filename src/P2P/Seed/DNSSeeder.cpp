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
			"mainnet.seed.grin.icu",			// gary.peverell@protonmail.com
			"mainnet.seed.713.mw",				// jasper@713.mw
			"mainnet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"mainnet.seed.grin.prokapi.com",	// hendi@prokapi.com
			"grinseed.yeastplume.org",			// yeastplume@protonmail.com
			"mainnet-seed.grinnode.live"	    // info@grinnode.live
		};
	}
	else
	{
		dnsSeeds = {
			"floonet.seed.grin.icu",			// gary.peverell@protonmail.com
			"floonet.seed.713.mw",				// jasper@713.mw
			"floonet.seed.grin.lesceller.com",	// q.lesceller@gmail.com
			"floonet.seed.grin.prokapi.com",	// hendi@prokapi.com
		};
	}

	for (auto seed : dnsSeeds)
	{
		LOG_TRACE_F("Checking seed: {}", seed);
		const std::vector<IPAddress> ipAddresses = Resolve(seed);
		for (const IPAddress ipAddress : ipAddresses)
		{
			LOG_TRACE_F("IP Address: {}", ipAddress);
			addresses.emplace_back(SocketAddress(ipAddress, Global::GetConfig().GetP2PPort()));
		}
	}

	return addresses;
}

std::vector<IPAddress> DNSSeeder::Resolve(const std::string& domainName)
{
	asio::io_context context;
	asio::ip::tcp::resolver resolver(context);
	asio::ip::tcp::resolver::query query(domainName, "domain");
	asio::error_code errorCode;
	asio::ip::tcp::resolver::iterator iter = resolver.resolve(query, errorCode);

	std::vector<IPAddress> addresses;
	if (!errorCode)
	{
		std::for_each(iter, {}, [&addresses](auto& it)
			{
				try
				{
					addresses.push_back(IPAddress(it.endpoint().address()));
				}
				catch (std::exception& e)
				{
					LOG_INFO_F("Exception thrown: {}", e.what());
				}
			});
	}
	else
	{
		LOG_TRACE_F("Error: {}", errorCode.message());
	}
	
	return addresses;
}