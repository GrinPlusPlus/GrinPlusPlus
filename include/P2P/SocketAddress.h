#pragma once

#include <P2P/IPAddress.h>

#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class SocketAddress
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

	SocketAddress(const SocketAddress& other) = default;
	SocketAddress(SocketAddress&& other) noexcept = default;

	//
	// Operators
	//
	SocketAddress& operator=(const SocketAddress& other) = default;
	SocketAddress& operator=(SocketAddress&& other) noexcept = default;

	//
	// Getters
	//
	inline const IPAddress& GetIPAddress() const { return m_ipAddress; }
	inline const uint16_t GetPortNumber() const { return m_port; }
	inline std::string Format() const
	{
		return m_ipAddress.Format() + ":" + std::to_string(m_port);
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