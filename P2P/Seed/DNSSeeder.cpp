#include "DNSSeeder.h"
#include "../IPAddressUtil.h"
#include "../Common.h"

#include <VectorUtil.h>
#pragma comment(lib , "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <Infrastructure/Logger.h>

bool DNSSeeder::GetPeersFromDNS(std::vector<SocketAddress>& addresses) const
{
	if (InitializeWinsock())
	{
		std::vector<std::string> dnsSeeds;
		
		// MAINNET: Support mainnet seeds
		dnsSeeds.emplace_back("floonet.seed.grin-tech.org"); // igno.peverell@protonmail.com
		dnsSeeds.emplace_back("floonet.seed.grin.icu"); // gary.peverell@protonmail.com

		for (auto seed : dnsSeeds)
		{
			GetAddressesFromDNSSeed(seed, addresses);
			//if (GetAddressesFromDNSSeed(seed, addresses))
			//{
			//	return true;
			//}
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

	int nMaxSolutions = 100;

	struct addrinfo *aiTrav = aiRes;
	while (aiTrav != nullptr && (nMaxSolutions == 0 || addresses.size() < nMaxSolutions))
	{
		if (aiTrav->ai_family == AF_INET)
		{
			const sockaddr_in* pSockAddrIn = (sockaddr_in*)aiTrav->ai_addr;
			const IPAddress address = IPAddressUtil::ParseIPAddress(pSockAddrIn->sin_addr);
			addresses.emplace_back(std::move(address), P2P::DEFAULT_PORT);

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