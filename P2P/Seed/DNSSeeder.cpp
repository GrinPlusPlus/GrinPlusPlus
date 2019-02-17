#include "DNSSeeder.h"
#include "../IPAddressUtil.h"
#include <P2P/Common.h>

#include <Common/Util/VectorUtil.h>
#pragma comment(lib , "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <Infrastructure/Logger.h>

DNSSeeder::DNSSeeder(const Config& config)
	: m_config(config)
{

}

bool DNSSeeder::GetPeersFromDNS(std::vector<SocketAddress>& addresses) const
{
	if (InitializeWinsock())
	{
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
			GetAddressesFromDNSSeed(seed, addresses);
		}

		return true;
	}

	return false;
}

bool DNSSeeder::GetAddressesFromDNSSeed(const std::string& dnsSeed, std::vector<SocketAddress>& addresses) const
{
	LoggerAPI::LogInfo("Retrieving seed nodes from dns:" + dnsSeed);
	bool hostsFound(false);

	WSADATA wsadata;
	int error = WSAStartup(0x0202, &wsadata);
	if (error)
	{
		return false;
	}

	struct addrinfo aiHint;
	memset(&aiHint, 0, sizeof(struct addrinfo));

	aiHint.ai_socktype = SOCK_STREAM;
	aiHint.ai_protocol = IPPROTO_TCP;
	aiHint.ai_family = AF_INET;
	aiHint.ai_flags = 0;
	struct addrinfo *aiRes = nullptr;
	int nErr = getaddrinfo(dnsSeed.c_str(), nullptr, &aiHint, &aiRes);
	if (nErr)
	{
		return false;
	}

	struct addrinfo *aiTrav = aiRes;
	while (aiTrav != nullptr)
	{
		if (aiTrav->ai_family == AF_INET)
		{
			const sockaddr_in* pSockAddrIn = (sockaddr_in*)aiTrav->ai_addr;
			const IPAddress address = IPAddressUtil::ParseIPAddress(pSockAddrIn->sin_addr);
			addresses.emplace_back(std::move(address), m_config.GetEnvironment().GetP2PPort());

			hostsFound = true;
		}

		aiTrav = aiTrav->ai_next;
	}

	freeaddrinfo(aiRes);

	return hostsFound;
}

bool DNSSeeder::InitializeWinsock() const
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// If WinSock isn't initialized, initialize it now.
	if (s == INVALID_SOCKET && WSAGetLastError() == WSANOTINITIALISED)
	{
		WSADATA wsadata;
		int error = WSAStartup(0x0202, &wsadata);

		if (error)
		{
			return false;
		}

		// Did we get the right Winsock version?
		if (wsadata.wVersion != 0x0202)
		{
			WSACleanup();
			return false;
		}
	}

	closesocket(s);
	return true;
}

IPAddress DNSSeeder::ParseIPAddress(const in_addr& ipv4Address) const
{
	const unsigned char byte1 = ipv4Address.S_un.S_un_b.s_b1;
	const unsigned char byte2 = ipv4Address.S_un.S_un_b.s_b2;
	const unsigned char byte3 = ipv4Address.S_un.S_un_b.s_b3;
	const unsigned char byte4 = ipv4Address.S_un.S_un_b.s_b4;

	const unsigned char bytes[] = { byte1, byte2, byte3, byte4 };
	std::vector<unsigned char> address = VectorUtil::MakeVector<unsigned char, 4>(bytes);

	return IPAddress(EAddressFamily::IPv4, address);
}