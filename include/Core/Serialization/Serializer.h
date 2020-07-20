#pragma once

#include <Common/Secure.h>
#include <Core/Serialization/EndianHelper.h>
#include <Core/Enums/ProtocolVersion.h>
#include <Crypto/BigInteger.h>

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>

enum class ESerializeLength
{
	U32,
	U64,
	NONE
};

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
		std::vector<uint8_t> temp(sizeof(T));
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
		std::vector<uint8_t> temp(sizeof(T));
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

	void AppendBytes(const std::vector<uint8_t>& vectorToAppend, const ESerializeLength prepend_length = ESerializeLength::NONE)
	{
		AppendLength(prepend_length, vectorToAppend.size());

		m_serialized.insert(m_serialized.end(), vectorToAppend.cbegin(), vectorToAppend.cend());
	}

	void AppendByteVector(const std::vector<uint8_t>& vectorToAppend, const ESerializeLength prepend_length = ESerializeLength::NONE)
	{
		AppendLength(prepend_length, vectorToAppend.size());

		m_serialized.insert(m_serialized.end(), vectorToAppend.cbegin(), vectorToAppend.cend());
	}

	void AppendByteVector(const SecureVector& vectorToAppend, const ESerializeLength prepend_length = ESerializeLength::NONE)
	{
		AppendLength(prepend_length, vectorToAppend.size());

		m_serialized.insert(m_serialized.end(), vectorToAppend.cbegin(), vectorToAppend.cend());
	}

	void AppendVarStr(const std::string& varString)
	{
		AppendLength(ESerializeLength::U64, varString.length());

		m_serialized.insert(m_serialized.end(), varString.cbegin(), varString.cend());
	}

	void AppendStr(const std::string& str, const ESerializeLength prepend_length = ESerializeLength::NONE)
	{
		AppendLength(prepend_length, str.length());

		m_serialized.insert(m_serialized.end(), str.cbegin(), str.cend());
	}

	template<size_t NUM_BYTES>
	void AppendBigInteger(const CBigInteger<NUM_BYTES>& bigInteger)
	{
		const std::vector<uint8_t>& data = bigInteger.GetData();
		//std::reverse(data.begin(), data.end());
		m_serialized.insert(m_serialized.end(), data.cbegin(), data.cend());
	}

	const std::vector<uint8_t>& GetBytes() const { return m_serialized; }
	EProtocolVersion GetProtocolVersion() const noexcept { return m_protocolVersion; }

	const uint8_t* data() const { return m_serialized.data(); }
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
	void AppendLength(const ESerializeLength prepend_length, const size_t length)
	{
		if (prepend_length == ESerializeLength::U64) {
			Append<uint64_t>(length);
		} else if (prepend_length == ESerializeLength::U32) {
			Append<uint32_t>((uint32_t)length);
		}
	}

	EProtocolVersion m_protocolVersion;
	std::vector<uint8_t> m_serialized;
};