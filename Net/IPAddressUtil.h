#pragma once

#include <Net/IPAddress.h>
#include <WinSock2.h>

class IPAddressUtil
{
public:
	//
	// Reads the IP address from the given in_addr, and return an IPAddress structure.
	//
	static IPAddress ParseIPAddress(const IN_ADDR& ipv4Address);
};