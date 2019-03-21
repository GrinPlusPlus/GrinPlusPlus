#include <Net/DNS.h>

#include "IPAddressUtil.h"

#pragma comment(lib , "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>

std::vector<IPAddress> DNS::Resolve(const std::string& domainName)
{
	std::vector<IPAddress> addresses;
	if (InitializeWinsock())
	{
		WSADATA wsadata;
		int error = WSAStartup(0x0202, &wsadata);
		if (error)
		{
			return addresses;
		}

		struct addrinfo aiHint;
		memset(&aiHint, 0, sizeof(struct addrinfo));

		aiHint.ai_socktype = SOCK_STREAM;
		aiHint.ai_protocol = IPPROTO_TCP;
		aiHint.ai_family = AF_INET;
		aiHint.ai_flags = 0;
		struct addrinfo *aiRes = nullptr;
		int nErr = getaddrinfo(domainName.c_str(), nullptr, &aiHint, &aiRes);
		if (nErr)
		{
			return addresses;
		}

		struct addrinfo *aiTrav = aiRes;
		while (aiTrav != nullptr)
		{
			if (aiTrav->ai_family == AF_INET)
			{
				const sockaddr_in* pSockAddrIn = (sockaddr_in*)aiTrav->ai_addr;
				addresses.emplace_back(IPAddressUtil::ParseIPAddress(pSockAddrIn->sin_addr));
			}

			aiTrav = aiTrav->ai_next;
		}

		freeaddrinfo(aiRes);
	}

	return addresses;
}

bool DNS::InitializeWinsock()
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