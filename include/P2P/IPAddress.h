#pragma once

#include <vector>
#include <BitUtil.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

enum class EAddressFamily
{
	IPv4,
	IPv6
};

class IPAddress
{
public:
	//
	// Constructors
	//
	IPAddress(const EAddressFamily& family, std::vector<unsigned char>&& address)
		: m_family(family), m_address(std::move(address))
	{

	}

	IPAddress(const EAddressFamily& family, const std::vector<unsigned char>& address)
		: m_family(family), m_address(address)
	{

	}

	IPAddress(const IPAddress& other) = default;
	IPAddress(IPAddress&& other) noexcept = default;
	IPAddress() = default;

	static IPAddress FromIP(const uint8_t byte1, const uint8_t byte2, const uint8_t byte3, const uint8_t byte4)
	{
		return IPAddress(EAddressFamily::IPv4, std::vector<unsigned char>({ byte1, byte2, byte3, byte4 }));
	}

	//
	// Destructor
	//
	~IPAddress() = default;

	//
	// Operators
	//
	IPAddress& operator=(const IPAddress& other) = default;
	IPAddress& operator=(IPAddress&& other) noexcept = default;

	inline bool operator< (const IPAddress& rhs) const {
		for (size_t i = 0; i < 4; i++)
		{
			if (m_address[i] != rhs.m_address[i])
			{
				return m_address[i] < rhs.m_address[i];
			}
		}

		return false;
		//// std::tuple's lexicographic ordering does all the actual work for you
		//// and using std::tie means no actual copies are made
		//return std::tie(m_address) < std::tie(rhs.m_address);
	}

	inline bool operator==(const IPAddress& rhs) const
	{
		if (m_address == rhs.m_address)
		{
			return true;
		}

		return false;
	}

	//
	// Getters
	//
	inline const EAddressFamily GetFamily() const { return m_family; }
	inline const std::vector<unsigned char>& GetAddress() const { return m_address; }
	inline std::string Format() const 
	{
		if (m_family == EAddressFamily::IPv4)
		{
			return std::to_string(m_address[0]) + "." + std::to_string(m_address[1]) + "." 
				+ std::to_string(m_address[2]) + "." + std::to_string(m_address[3]);
		}
		else
		{
			return std::to_string(m_address[0]) + ":" + std::to_string(m_address[1]) + ":" + std::to_string(m_address[2]) + ":" 
				+ std::to_string(m_address[3]) + ":" + std::to_string(m_address[4]) + ":" + std::to_string(m_address[5]);
		}
	}

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		if (m_family == EAddressFamily::IPv4)
		{
			serializer.Append<uint8_t>(0);
		}
		else
		{
			serializer.Append<uint8_t>(1);
		}

		serializer.AppendByteVector(m_address);
	}

	static IPAddress Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t ipAddressFamily = byteBuffer.ReadU8();

		if (ipAddressFamily == 0)
		{
			std::vector<unsigned char> ipAddress = byteBuffer.ReadVector(4);
			return IPAddress(EAddressFamily::IPv4, std::move(ipAddress));
		}
		else
		{
			std::vector<unsigned char> ipAddress = byteBuffer.ReadVector(16);
			return IPAddress(EAddressFamily::IPv6, std::move(ipAddress));
		}
	}

private:
	EAddressFamily m_family;
	std::vector<unsigned char> m_address; // Will contain 4 bytes for IPv4, and 16 bytes for IPv6 (in big-endian order).
};

namespace std
{
	template<>
	struct hash<IPAddress>
	{
		size_t operator()(const IPAddress& address) const
		{
			const std::vector<unsigned char>& bytes = address.GetAddress();

			if (address.GetFamily() == EAddressFamily::IPv4)
			{
				return BitUtil::ConvertToU32(bytes[0], bytes[1], bytes[2], bytes[3]);
			}
			else
			{
				// TODO: Implement
				return 0;
			}
		}
	};
}