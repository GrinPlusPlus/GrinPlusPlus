#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Serialization/EndianHelper.h>
#include <Core/Serialization/DeserializationException.h>

#include <vector>
#include <string>
#include <cstring>
#include <stdint.h>
#include <algorithm>

#include <Crypto/BigInteger.h>

class ByteBuffer
{
public:
	ByteBuffer(const std::vector<unsigned char>& bytes)
		: m_bytes(bytes), m_index(0)
	{

	}

	template<class T>
	void ReadBigEndian(T& t)
	{
		if (m_index + sizeof(T) > m_bytes.size())
		{
			throw DeserializationException();
		}

		if (EndianHelper::IsBigEndian())
		{
			memcpy(&t, &m_bytes[m_index], sizeof(T));
		}
		else
		{
			std::vector<unsigned char> temp;
			temp.resize(sizeof(T));
			std::reverse_copy(m_bytes.cbegin() + m_index, m_bytes.cbegin() + m_index + sizeof(T), temp.begin());
			memcpy(&t, &temp[0], sizeof(T));
		}

		m_index += sizeof(T);
	}

	template<class T>
	void ReadLittleEndian(T& t)
	{
		if (m_index + sizeof(T) > m_bytes.size())
		{
			throw DeserializationException();
		}

		if (EndianHelper::IsBigEndian())
		{
			std::vector<unsigned char> temp;
			temp.resize(sizeof(T));
			std::reverse_copy(m_bytes.cbegin() + m_index, m_bytes.cbegin() + m_index + sizeof(T), temp.begin());
			memcpy(&t, &temp[0], sizeof(T));
		}
		else
		{
			memcpy(&t, &m_bytes[m_index], sizeof(T));
		}

		m_index += sizeof(T);
	}

	int8_t Read8()
	{
		int8_t value;
		ReadBigEndian(value);

		return value;
	}

	uint8_t ReadU8()
	{
		uint8_t value;
		ReadBigEndian(value);

		return value;
	}

	int16_t Read16()
	{
		int16_t value;
		ReadBigEndian(value);

		return value;
	}

	uint16_t ReadU16()
	{
		uint16_t value;
		ReadBigEndian(value);

		return value;
	}

	int32_t Read32()
	{
		int32_t value;
		ReadBigEndian(value);

		return value;
	}

	uint32_t ReadU32()
	{
		uint32_t value;
		ReadBigEndian(value);

		return value;
	}

	int64_t Read64()
	{
		int64_t value;
		ReadBigEndian(value);

		return value;
	}

	uint64_t ReadU64()
	{
		uint64_t value;
		ReadBigEndian(value);

		return value;
	}

	uint64_t ReadU64_LE()
	{
		uint64_t value;
		ReadLittleEndian(value);

		return value;
	}

	std::string ReadVarStr()
	{
		const uint64_t stringLength = ReadU64();
		if (stringLength == 0)
		{
			return "";
		}

		if (m_index + stringLength > m_bytes.size())
		{
			throw DeserializationException();
		}

		std::vector<unsigned char> temp(m_bytes.cbegin() + m_index, m_bytes.cbegin() + m_index + stringLength);
		m_index += stringLength;

		return std::string((char*)&temp[0], stringLength);
	}

	template<size_t NUM_BYTES>
	CBigInteger<NUM_BYTES> ReadBigInteger()
	{
		if (m_index + NUM_BYTES > m_bytes.size())
		{
			throw DeserializationException();
		}

		std::vector<unsigned char> data(m_bytes.cbegin() + m_index, m_bytes.cbegin() + m_index + NUM_BYTES);

		m_index += NUM_BYTES;

		return CBigInteger<NUM_BYTES>(data);
	}

	std::vector<unsigned char> ReadVector(const uint64_t numBytes)
	{
		if (m_index + numBytes > m_bytes.size())
		{
			throw DeserializationException();
		}

		const size_t index = m_index;
		m_index += numBytes;

		return std::vector<unsigned char>(m_bytes.cbegin() + index, m_bytes.cbegin() + index + numBytes);
	}

	inline size_t GetRemainingSize() const
	{
		return m_bytes.size() - m_index;
	}

private:
	size_t m_index;
	const std::vector<unsigned char>& m_bytes;
};
