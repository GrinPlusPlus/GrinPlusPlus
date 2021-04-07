#pragma once

#include <Net/IPAddress.h>

#include <Core/Traits/Printable.h>
#include <Core/Traits/Serializable.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class SocketAddress : public Traits::IPrintable, public Traits::ISerializable
{
public:
	//
	// Constructors
	//
	SocketAddress(IPAddress&& ipAddress, const uint16_t port)
		: m_ipAddress(std::move(ipAddress)), m_port(port)
	{

	}

	SocketAddress(const IPAddress& ipAddress, const uint16_t port)
		: m_ipAddress(ipAddress), m_port(port)
	{

	}

	SocketAddress(const std::string& ipAddress, const uint16_t port)
		: m_ipAddress(IPAddress::Parse(ipAddress)), m_port(port)
	{

	}

	SocketAddress(const SocketAddress& other) = default;
	SocketAddress(SocketAddress&& other) noexcept = default;

	static SocketAddress Parse(const std::string& addressStr)
	{
		auto parts = StringUtil::Split(addressStr, ":");
		if (parts.size() != 2)
		{
			throw DESERIALIZATION_EXCEPTION_F("Failed to parse SocketAddress: {}", addressStr);
		}

		IPAddress ipAddress = IPAddress::Parse(parts[0]);
		uint16_t port = (uint16_t)std::stoi(parts[1]);

		return SocketAddress(std::move(ipAddress), port);
	}

	static SocketAddress CreateV4(const std::array<uint8_t, 4>& bytes, const uint16_t port)
	{
		return SocketAddress(IPAddress::CreateV4(bytes), port);
	}

	static SocketAddress CreateV6(const std::array<uint8_t, 16>& bytes, const uint16_t port)
	{
		return SocketAddress(IPAddress::CreateV6(bytes), port);
	}

	static SocketAddress From(const asio::ip::tcp::socket& socket)
	{
		return SocketAddress::FromEndpoint(socket.remote_endpoint());
	}

	static SocketAddress FromEndpoint(const asio::ip::tcp::endpoint& endpoint)
	{
		return SocketAddress(endpoint.address().to_string(), endpoint.port());
	}

	//
	// Destructor
	//
	virtual ~SocketAddress() = default;

	//
	// Operators
	//
	SocketAddress& operator=(const SocketAddress& other) = default;
	SocketAddress& operator=(SocketAddress&& other) noexcept = default;
	bool operator==(const SocketAddress& rhs) const {
		return m_ipAddress == rhs.m_ipAddress && m_port == rhs.m_port;
	}
	bool operator<(const SocketAddress& rhs) const {
		return m_ipAddress < rhs.m_ipAddress || (m_ipAddress == rhs.m_ipAddress && m_port < rhs.m_port);
	}

	//
	// Getters
	//
	const IPAddress& GetIPAddress() const { return m_ipAddress; }
	const uint16_t GetPortNumber() const { return m_port; }
	std::string Format() const final
	{
		return m_ipAddress.Format() + ":" + std::to_string(m_port);
	}

	asio::ip::tcp::endpoint GetEndpoint() const
	{
		return asio::ip::tcp::endpoint{
			asio::ip::address_v4::from_string(m_ipAddress.Format()),
			m_port
		};
	}

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		m_ipAddress.Serialize(serializer);

		serializer.Append<uint16_t>(m_port);
	}

	static SocketAddress Deserialize(ByteBuffer& byteBuffer)
	{
		IPAddress ipAddress = IPAddress::Deserialize(byteBuffer);
		const uint16_t port = byteBuffer.ReadU16();

		return SocketAddress(std::move(ipAddress), port);
	}

private:
	IPAddress m_ipAddress;
	uint16_t m_port;
};