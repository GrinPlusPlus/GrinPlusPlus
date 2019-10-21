#pragma once

#include <vector>
#include <Common/Util/BitUtil.h>
#include <Common/Util/HexUtil.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

enum class EAddressFamily
{
	IPv4,
	IPv6
};

// TODO: Implement IPv6
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

	static IPAddress FromString(const std::string& addressStr)
	{
		uint32_t byte1;
		uint32_t byte2;
		uint32_t byte3;
		uint32_t byte4;
		#ifdef _WIN32
		sscanf_s(addressStr.c_str(), "%u.%u.%u.%u", &byte1, &byte2, &byte3, &byte4);
		#else
		sscanf(addressStr.c_str(), "%u.%u.%u.%u", &byte1, &byte2, &byte3, &byte4);
		#endif

		return FromIP((uint8_t)byte1, (uint8_t)byte2, (uint8_t)byte3, (uint8_t)byte4);
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
	inline bool IsLocalhost() const
	{
		if (m_family == EAddressFamily::IPv4)
		{
			return m_address[0] == 127 && m_address[1] == 0 && m_address[2] == 0 && m_address[3] == 1;
		}
		else
		{
			for (size_t i = 0; i < 15; i++)
			{
				if (m_address[i] != 0)
				{
					return false;
				}
			}

			return m_address[15] == 1;
		}
	}
	inline std::string Format() const 
	{
		if (m_address.empty())
		{
			return "";
		}

		if (m_family == EAddressFamily::IPv4)
		{
			return std::to_string(m_address[0]) + "." + std::to_string(m_address[1]) + "." 
				+ std::to_string(m_address[2]) + "." + std::to_string(m_address[3]);
		}
		else
		{
			const uint16_t words[8] = { BitUtil::ConvertToU16(m_address[0], m_address[1]), BitUtil::ConvertToU16(m_address[2], m_address[3]), 
				BitUtil::ConvertToU16(m_address[4], m_address[5]), BitUtil::ConvertToU16(m_address[6], m_address[7]), BitUtil::ConvertToU16(m_address[8], m_address[9]), 
				BitUtil::ConvertToU16(m_address[10], m_address[11]), BitUtil::ConvertToU16(m_address[12], m_address[13]), BitUtil::ConvertToU16(m_address[14], m_address[15]) };

			std::string formatted;
			for (int i = 0; i < 8; i++)
			{
				if (i > 0)
				{
					formatted += ":";
				}

				if (words[i] == 0 && i < 8 && words[i + 1] == 0)
				{
					while (words[i + 1] == 0)
					{
						++i;
					}

					continue;
				}

				formatted += HexUtil::ConvertToHex(words[i], true);
			}

			return formatted;
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
		else if (ipAddressFamily == 1)
		{
			std::vector<unsigned char> ipAddress = byteBuffer.ReadVector(16);
			return IPAddress(EAddressFamily::IPv6, std::move(ipAddress));
		}
		else
		{
			throw DeserializationException();
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