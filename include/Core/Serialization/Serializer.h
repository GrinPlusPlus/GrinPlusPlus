#pragma once

#include <Common/Secure.h>
#include <Core/Serialization/EndianHelper.h>
#include <Core/Enums/ProtocolVersion.h>
#include <Crypto/BigInteger.h>

#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>

class Serializer
{
public:
	Serializer(const EProtocolVersion protocolVersion = EProtocolVersion::V1)
		: m_protocolVersion(protocolVersion) { }
	Serializer(const size_t expectedSize, const EProtocolVersion protocolVersion = EProtocolVersion::V1)
		: m_protocolVersion(protocolVersion)
	{
		m_serialized.reserve(expectedSize);
	}

	template <class T>
	void Append(const T& t)
	{
		std::vector<unsigned char> temp(sizeof(T));
		memcpy(&temp[0], &t, sizeof(T));

		if (EndianHelper::IsBigEndian())
		{
			m_serialized.insert(m_serialized.end(), temp.cbegin(), temp.cend());
		}
		else
		{
			m_serialized.insert(m_serialized.end(), temp.crbegin(), temp.crend());
		}
	}
	template <class T>
	void AppendLittleEndian(const T& t)
	{
		std::vector<unsigned char> temp(sizeof(T));
		memcpy(&temp[0], &t, sizeof(T));

		if (EndianHelper::IsBigEndian())
		{
			m_serialized.insert(m_serialized.end(), temp.crbegin(), temp.crend());
		}
		else
		{
			m_serialized.insert(m_serialized.end(), temp.cbegin(), temp.cend());
		}
	}

	void AppendByteVector(const std::vector<unsigned char>& vectorToAppend)
	{
		m_serialized.insert(m_serialized.end(), vectorToAppend.cbegin(), vectorToAppend.cend());
	}

	void AppendByteVector(const SecureVector& vectorToAppend)
	{
		m_serialized.insert(m_serialized.end(), vectorToAppend.cbegin(), vectorToAppend.cend());
	}

	void AppendVarStr(const std::string& varString)
	{
		size_t stringLength = varString.length();
		Append<uint64_t>(stringLength);
		m_serialized.insert(m_serialized.end(), varString.cbegin(), varString.cend());
	}

	void AppendFixedStr(const std::string& str)
	{
		m_serialized.insert(m_serialized.end(), str.cbegin(), str.cend());
	}

	template<size_t NUM_BYTES>
	void AppendBigInteger(const CBigInteger<NUM_BYTES>& bigInteger)
	{
		const std::vector<unsigned char>& data = bigInteger.GetData();
		//std::reverse(data.begin(), data.end());
		m_serialized.insert(m_serialized.end(), data.cbegin(), data.cend());
	}

	const std::vector<unsigned char>& GetBytes() const { return m_serialized; }
	EProtocolVersion GetProtocolVersion() const noexcept { return m_protocolVersion; }

	const unsigned char* data() const { return m_serialized.data(); }
	size_t size() const { return m_serialized.size(); }

	// WARNING: This will destroy the contents of m_serialized.
	// TODO: Create a SecureSerializer instead.
	SecureVector GetSecureBytes()
	{
		SecureVector secureBytes(m_serialized.begin(), m_serialized.end());
		cleanse(m_serialized.data(), m_serialized.size());

		return secureBytes;
	}

private:
	EProtocolVersion m_protocolVersion;
	std::vector<unsigned char> m_serialized;
};