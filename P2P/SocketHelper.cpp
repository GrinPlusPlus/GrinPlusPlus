#include "SocketHelper.h"

#include <WS2tcpip.h>

std::optional<SOCKET> SocketHelper::Connect(const IPAddress& ipAddress, const uint16_t portNumber)
{
	if (ipAddress.GetFamily() == EAddressFamily::IPv4)
	{
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock != INVALID_SOCKET)
		{
			sockaddr_in sa;
			sa.sin_family = AF_INET;
			sa.sin_port = htons(portNumber);

			const std::vector<unsigned char>& bytes = ipAddress.GetAddress();
			sa.sin_addr.S_un.S_un_b = { bytes[0], bytes[1], bytes[2], bytes[3] };

			if (Connect(sock, (SOCKADDR&)sa))
			{
				return std::make_optional<SOCKET>(sock);
			}

			closesocket(sock);
		}
	}
	else
	{
		SOCKET sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (sock != INVALID_SOCKET)
		{
			sockaddr_in6 sa;
			sa.sin6_family = AF_INET6;
			sa.sin6_port = htons(portNumber);
			memcpy(&sa.sin6_addr.u.Byte[0], &ipAddress.GetAddress()[0], 16);

			if (Connect(sock, (SOCKADDR&)sa))
			{
				return std::make_optional<SOCKET>(sock);
			}

			closesocket(sock);
		}
	}

	return std::nullopt;
}

std::optional<uint16_t> SocketHelper::GetPort(SOCKET socket)
{
	// TODO: Handle IPv6
	struct sockaddr_in sin;
	int addrlen = sizeof(sin);
	if (getsockname(socket, (struct sockaddr *)&sin, &addrlen) == 0)
	{
		if (sin.sin_family == AF_INET && addrlen == sizeof(sin))
		{
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(sin.sin_addr), str, INET_ADDRSTRLEN);

			return std::make_optional<uint16_t>(ntohs(sin.sin_port));
		}
	}

	return std::nullopt;
}

bool SocketHelper::Connect(SOCKET socket, SOCKADDR& socketAddress)
{
	unsigned long nonblocking = 1;
	if (ioctlsocket(socket, FIONBIO, &nonblocking) == SOCKET_ERROR)
	{
		return false;
	}

	if (connect(socket, (SOCKADDR*)&socketAddress, sizeof(socketAddress)) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			// connection failed
			return false;
		}

		// connection pending
		fd_set setW, setE;

		FD_ZERO(&setW);
		FD_SET(socket, &setW);
		FD_ZERO(&setE);
		FD_SET(socket, &setE);

		timeval time_out = { 0 };
		time_out.tv_sec = 0;
		time_out.tv_usec = 300000;

		int ret = select(0, NULL, &setW, &setE, &time_out);
		if (ret <= 0)
		{
			// select() failed or connection timed out
			if (ret == 0)
			{
				WSASetLastError(WSAETIMEDOUT);
			}

			return false;
		}

		if (FD_ISSET(socket, &setE))
		{
			// connection failed
			int error = 0;
			int size = sizeof(error);
			getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&error, &size);
			WSASetLastError(error);
			return false;
		}
	}

	// connection successful - put socket in blocking mode...
	unsigned long block = 0;
	if (ioctlsocket(socket, FIONBIO, &block) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}