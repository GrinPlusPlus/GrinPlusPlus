#include <Net/Listener.h>
#include <Net/SocketFactory.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>

Listener::Listener(const SOCKET& socket)
	: m_socket(socket)
{

}

std::optional<Listener> Listener::Create(const IPAddress& address, const uint16_t portNumber)
{
	WSADATA wsadata;
	int error = WSAStartup(0x0202, &wsadata);
	if (error)
	{
		return std::nullopt;
	}

	if (address.GetFamily() == EAddressFamily::IPv4)
	{
		const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket != INVALID_SOCKET)
		{
			sockaddr_in service;
			service.sin_family = AF_INET;
			service.sin_port = htons(portNumber);
			service.sin_addr.S_un.S_un_b = { 0, 0, 0, 0 }; // TODO: Use address

			const int bindResult = bind(listenSocket, (SOCKADDR *)&service, sizeof(service));
			if (bindResult != SOCKET_ERROR)
			{
				if (listen(listenSocket, SOMAXCONN) != SOCKET_ERROR)
				{
					unsigned long nonblocking = 1;
					if (ioctlsocket(listenSocket, FIONBIO, &nonblocking) != SOCKET_ERROR)
					{
						return std::make_optional(Listener(listenSocket));
					}
				}
			}
		}
	}
	else
	{
		const SOCKET listenSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket != INVALID_SOCKET)
		{
			sockaddr_in6 service;
			service.sin6_family = AF_INET6;
			service.sin6_port = htons(portNumber);
			memcpy(&service.sin6_addr.u.Byte[0], &address.GetAddress()[0], 16);

			const int bindResult = bind(listenSocket, (SOCKADDR *)&service, sizeof(service));
			if (bindResult != SOCKET_ERROR)
			{
				if (listen(listenSocket, SOMAXCONN) != SOCKET_ERROR)
				{
					unsigned long nonblocking = 1;
					if (ioctlsocket(listenSocket, FIONBIO, &nonblocking) != SOCKET_ERROR)
					{
						return std::make_optional(Listener(listenSocket));
					}
				}
			}

			closesocket(listenSocket);
		}
	}

	return std::nullopt;
}

bool Listener::Close()
{
	return closesocket(m_socket) != SOCKET_ERROR;
}

std::optional<Socket> Listener::AcceptNewConnection() const
{
	const SOCKET acceptedSocket = accept(m_socket, nullptr, nullptr);
	if (acceptedSocket != SOCKET_ERROR)
	{
		return SocketFactory::CreateSocket(acceptedSocket);
	}

	return std::nullopt;
}