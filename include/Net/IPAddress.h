#pragma once

#include <vector>
#include <Core/Traits/Printable.h>
#include <Common/Util/BitUtil.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

enum class EAddressFamily
{
	IPv4,
	IPv6
};

// FUTURE: Implement IPv6
class IPAddress : public Traits::IPrintable
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
	IPAddress() : m_family(EAddressFamily::IPv4), m_address() { }

	static IPAddress FromIP(const uint8_t byte1, const uint8_t byte2, const uint8_t byte3, const uint8_t byte4)
	{
		return IPAddress(EAddressFamily::IPv4, std::vector<unsigned char>({ byte1, byte2, byte3, byte4 }));
	}

	static IPAddress FromString(const std::string& addressStr)
	{
		try
		{
			std::vector<std::string> octets = StringUtil::Split(addressStr, ".");
			if (octets.size() == 4)
			{
				uint8_t byte1 = (uint8_t)std::stoi(octets[0], nullptr, 10);
				uint8_t byte2 = (uint8_t)std::stoi(octets[1], nullptr, 10);
				uint8_t byte3 = (uint8_t)std::stoi(octets[2], nullptr, 10);
				uint8_t byte4 = (uint8_t)std::stoi(octets[3], nullptr, 10);

				return FromIP(byte1, byte2, byte3, byte4);
			}
		}
		catch (...)
		{
		}

		throw DESERIALIZATION_EXCEPTION();
	}

	//
	// Destructor
	//
	virtual ~IPAddress() = default;

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

	virtual std::string Format() const override final
	{
		if (m_address.empty())
		{
			return "";
		}

		if (m_family == EAddressFamily::IPv4)
		{
			return StringUtil::Format("{}.{}.{}.{}", m_address[0], m_address[1], m_address[2], m_address[3]);
		}
		else
		{
			std::vector<uint16_t> words;
			for (int i = 0; i < 16; i += 2)
			{
				words.push_back(BitUtil::ConvertToU16(m_address[i], m_address[i + 1]));
			}

			std::string formatted;
			for (int i = 0; i < 8; i++)
			{
				if (i > 0)
				{
					formatted += ":";
				}

				if (words[i] == 0 && i < 7 && words[i + 1] == 0)
				{
					while (words[i + 1] == 0)
					{
						++i;
					}

					continue;
				}

				formatted += HexUtil::ConvertToHex(words[i]);
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
			throw DESERIALIZATION_EXCEPTION();
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
				// TODO: Not optimal, but Grin IPv6 support isn't implemented yet.
				return BitUtil::ConvertToU32(bytes[12], bytes[13], bytes[14], bytes[15]);
			}
		}
	};
}