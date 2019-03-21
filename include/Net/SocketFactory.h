#pragma once

#include <Net/Socket.h>
#include <optional>

// Forward Declarations
typedef unsigned long long SOCKET;
typedef struct sockaddr SOCKADDR;

class SocketFactory
{
public:
	static std::optional<Socket> Connect(const IPAddress& ipAddress, const uint16_t portNumber);
	static std::optional<Socket> CreateSocket(SOCKET socket);

private:
	static bool Connect(SOCKET socket, SOCKADDR& socketAddress);
};