#pragma once

#include <Net/IPAddress.h>
#include <vector>
#include <string>

class DNS
{
public:
	static std::vector<IPAddress> Resolve(const std::string& domainName);

private:
	static bool InitializeWinsock();
};