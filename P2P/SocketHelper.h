#pragma once

#include <P2P/IPAddress.h>
#include <P2P/SocketAddress.h>

#include <optional>
#include <WinSock2.h>

class SocketHelper
{
public:
	static std::optional<SOCKET> Connect(const IPAddress& ipAddress, const uint16_t portNumber);
	static std::optional<uint16_t> GetPort(SOCKET socket);
	static std::optional<SocketAddress> GetSocketAddress(SOCKET socket);
	static std::optional<SOCKET> CreateListener(const IPAddress& localHost, const uint16_t portNumber);
	static std::optional<SOCKET> AcceptNewConnection(const SOCKET listeningSocket);

private:
	static bool Connect(SOCKET socket, SOCKADDR& socketAddress);
};