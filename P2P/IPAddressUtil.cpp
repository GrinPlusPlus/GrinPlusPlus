#include "IPAddressUtil.h"

#include <Common/Util/VectorUtil.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

IPAddress IPAddressUtil::ParseIPAddress(const in_addr& ipv4Address)
{
	const unsigned char byte1 = ipv4Address.S_un.S_un_b.s_b1;
	const unsigned char byte2 = ipv4Address.S_un.S_un_b.s_b2;
	const unsigned char byte3 = ipv4Address.S_un.S_un_b.s_b3;
	const unsigned char byte4 = ipv4Address.S_un.S_un_b.s_b4;

	const unsigned char bytes[] = { byte1, byte2, byte3, byte4 };
	std::vector<unsigned char> address = VectorUtil::MakeVector<unsigned char, 4>(bytes);

	return IPAddress(EAddressFamily::IPv4, address);
}