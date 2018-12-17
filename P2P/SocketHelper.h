#pragma once

#include <P2P/IPAddress.h>

#include <optional>
#include <WinSock2.h>

class SocketHelper
{
public:
	static std::optional<SOCKET> Connect(const IPAddress& ipAddress, const uint16_t portNumber);
	static std::optional<uint16_t> GetPort(SOCKET socket);

private:
	static bool Connect(SOCKET socket, SOCKADDR& socketAddress);
};