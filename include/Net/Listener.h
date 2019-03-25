#pragma once

#include <Net/Socket.h>
#include <optional>

class Listener
{
public:
	static std::optional<Listener> Create(const IPAddress& address, const uint16_t portNumber);

	bool Close();
	std::optional<Socket> Listener::AcceptNewConnection() const;

private:
	Listener(const SOCKET& socket);

	SOCKET m_socket;
};