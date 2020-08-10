#pragma once

#include <Net/Clients/Client.h>
#include <Net/Clients/SOCKS/SOCKS.h>
#include <Net/Clients/SOCKS/SOCKSException.h>
#include <Net/Clients/HTTP/HTTPClient.h>

static const std::chrono::seconds SOCKS_TIMEOUT = std::chrono::seconds(3);

class SOCKSClient : public IHTTPClient
{
public:
	SOCKSClient(SocketAddress&& proxyAddress)
		: m_proxyAddress(std::move(proxyAddress))
	{

	}

	virtual ~SOCKSClient() = default;

private:
	void EstablishConnection(const std::string& destination, const uint16_t port) final
	{
		EstablishConnectionInternal(destination, port, 1);
	}

	void EstablishConnectionInternal(const std::string& destination, const uint16_t port, const int numAttempts)
	{
		WALLET_INFO_F("Connection attempt {}", numAttempts);
		try
		{
			Client::Connect(m_proxyAddress, std::chrono::seconds(15));

			if (destination.size() > 255)
			{
				throw SOCKS_EXCEPTION("Destination too long.");
			}

			Initialize(nullptr);
			Authenticate(nullptr);
			Connect(destination + ".onion", port);
		}
		catch (SOCKSException& e)
		{
			WALLET_INFO_F("SOCKSException: {}", e.what());

			if (numAttempts < 3)
			{
				EstablishConnectionInternal(destination, port, numAttempts + 1);
			}
			else
			{
				throw;
			}
		}
		catch (std::exception& e)
		{
			WALLET_INFO_F("Exception: {}", e.what());

			if (numAttempts < 5)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				EstablishConnectionInternal(destination, port, numAttempts + 1);
			}
			else
			{
				throw SOCKS_EXCEPTION(e.what());
			}
		}
	}

	void Initialize(const SOCKS::ProxyCredentials* auth)
	{
		// Accepted authentication methods
		std::vector<uint8_t> data;
		if (auth != nullptr)
		{
			data = std::vector<uint8_t>({ SOCKS::Version::SOCKS5, 0x02, SOCKS::Method::NOAUTH, SOCKS::Method::USER_PASS });
		}
		else
		{
			data = std::vector<uint8_t>({ SOCKS::Version::SOCKS5, 0x01, SOCKS::Method::NOAUTH });
		}

		Write(data, SOCKS_TIMEOUT);
	}

	void Authenticate(const SOCKS::ProxyCredentials* pAuth)
	{
		std::vector<uint8_t> received = Read(2, SOCKS_TIMEOUT);
		if (received[0] != SOCKS::Version::SOCKS5)
		{
			throw SOCKS_EXCEPTION("Proxy failed to initialize");
		}

		if (received[1] == SOCKS::Method::USER_PASS && pAuth != nullptr)
		{
			if (pAuth->username.size() > 255 || pAuth->password.size() > 255)
			{
				throw SOCKS_EXCEPTION("Proxy username or password too long");
			}

			// Perform username/password authentication (as described in RFC1929)
			std::vector<uint8_t> data;
			data.push_back(0x01); // Current (and only) version of user/pass subnegotiation
			data.push_back((uint8_t)pAuth->username.size());
			data.insert(data.end(), pAuth->username.begin(), pAuth->username.end());
			data.push_back((uint8_t)pAuth->password.size());
			data.insert(data.end(), pAuth->password.begin(), pAuth->password.end());
			Write(data, SOCKS_TIMEOUT);

			std::vector<uint8_t> authResponse = Read(2, SOCKS_TIMEOUT);
			if (authResponse[0] != 0x01 || authResponse[1] != 0x00)
			{
				throw SOCKS_EXCEPTION("Proxy authentication unsuccessful");
			}
		}
		else if (received[1] == SOCKS::Method::NOAUTH)
		{
			// Perform no authentication
		}
		else
		{
			throw SOCKS_EXCEPTION("Proxy requested wrong authentication method");
		}
	}

	bool Connect(const std::string& destination, const int port)
	{
		std::vector<uint8_t> data;
		data.push_back(SOCKS::Version::SOCKS5); // VER protocol version
		data.push_back(SOCKS::Command::CONNECT); // CMD CONNECT
		data.push_back(0x00); // RSV Reserved must be 0
		data.push_back(SOCKS::Atyp::DOMAINNAME); // ATYP DOMAINNAME
		data.push_back((uint8_t)destination.size()); // Length<=255 is checked at beginning of function
		data.insert(data.end(), destination.begin(), destination.end());
		data.push_back((port >> 8) & 0xFF);
		data.push_back((port >> 0) & 0xFF);
		Write(data, SOCKS_TIMEOUT);

		std::vector<uint8_t> connectResponse = Read(4, SOCKS_TIMEOUT);

		if (connectResponse[0] != SOCKS::Version::SOCKS5)
		{
			throw SOCKS_EXCEPTION("Proxy failed to accept request");
		}

		if (connectResponse[1] != SOCKS::Reply::SUCCEEDED)
		{
			// Failures to connect to a peer that are not proxy errors
			// TODO: throw SOCKS_EXCEPTION("Connect failed with error: " + SOCKS::ErrorString(connectResponse[1]));
		}

		if (connectResponse[2] != 0x00) // Reserved field must be 0
		{
			throw SOCKS_EXCEPTION("Error: malformed proxy response");
		}

		ReadDestination((SOCKS::Atyp)connectResponse[3]); // TODO: Ignore result?

		return true;
	}

	SOCKS::Destination ReadDestination(const SOCKS::Atyp& type)
	{
		SOCKS::Destination destination;

		switch (type)
		{
			case SOCKS::Atyp::IPV4:
			{
				std::array<uint8_t, 4> addressArr;
				std::vector<uint8_t> address = Read(4, SOCKS_TIMEOUT);
				std::copy_n(address.begin(), 4, addressArr.begin());
				destination.ipAddress = std::make_optional(IPAddress::CreateV4(addressArr));
				break;
			}
			case SOCKS::Atyp::IPV6:
			{
				Read(16, SOCKS_TIMEOUT); // Not currently supported
				break;
			}
			case SOCKS::Atyp::DOMAINNAME:
			{
				std::vector<uint8_t> length = Read(1, SOCKS_TIMEOUT);
				std::vector<uint8_t> address = Read(length[0], SOCKS_TIMEOUT);

				destination.domainName = std::make_optional(std::string(address.begin(), address.end()));
				break;
			}
			default:
			{
				throw SOCKS_EXCEPTION("Error: malformed proxy response");
			}
		}

		std::vector<uint8_t> portBytes = Read(2, SOCKS_TIMEOUT);
		destination.port = ByteBuffer(portBytes).ReadU16();

		return destination;
	}

	SocketAddress m_proxyAddress;
};