#pragma once

#include <P2P/IPAddress.h>

// Forward Declarations
struct in_addr;

class IPAddressUtil
{
public:
	//
	// Reads the IP address from the given in_addr, and return an IPAddress structure.
	//
	static IPAddress ParseIPAddress(const in_addr& ipv4Address);
};