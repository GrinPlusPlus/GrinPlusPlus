#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma warning(disable: 4505)

template<size_t NUM_BYTES, class ALLOC = std::allocator<unsigned char>>
class CBigInteger
{
public:
	//
	// Constructors
	//
	CBigInteger()
		: m_data(NUM_BYTES)
	{
	}

	CBigInteger(const std::vector<unsigned char, ALLOC>& data)
		: m_data(data)
	{

	}

	CBigInteger(std::vector<unsigned char, ALLOC>&& data)
		: m_data(std::move(data))
	{

	}

	CBigInteger(const unsigned char* data)
		: m_data(data, data + NUM_BYTES)
	{
	}

	CBigInteger(const CBigInteger& bigInteger)
	{
		m_data = bigInteger.GetData();
	}

	CBigInteger(CBigInteger&& bigInteger) noexcept = default;

	//
	// Destructor
	//
	~CBigInteger() = default;

	void erase()
	{
		for (size_t i = 0; i < NUM_BYTES; i++)
		{
			m_data[i] = 0;
		}
	}

	inline const std::vector<unsigned char, ALLOC>& GetData() const
	{
		return m_data;
	}

	static CBigInteger<NUM_BYTES, ALLOC> ValueOf(const unsigned char value)
	{
		std::vector<unsigned char, ALLOC> data(NUM_BYTES);
		data[NUM_BYTES - 1] = value;
		return CBigInteger<NUM_BYTES, ALLOC>(&data[0]);
	}

	static CBigInteger<NUM_BYTES, ALLOC> FromHex(const std::string& hex);

	static CBigInteger<NUM_BYTES, ALLOC> GetMaximumValue()
	{
		std::vector<unsigned char, ALLOC> data(NUM_BYTES);
		for (int i = 0; i < NUM_BYTES; i++)
		{
			data[i] = 0xFF;
		}

		return CBigInteger<NUM_BYTES, ALLOC>(&data[0]);
	}

	inline size_t size() const { return NUM_BYTES; }
	inline const unsigned char* data() const { return m_data.data(); }

	const unsigned char* ToCharArray() const { return &m_data[0]; }
	std::string ToHex() const
	{
		std::ostringstream stream;
		for (const unsigned char byte : m_data)
		{
			stream << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int)byte;
		}

		return stream.str();
	}

	//
	// Operators
	//
	CBigInteger& operator=(const CBigInteger& other) = default;
	CBigInteger& operator=(CBigInteger&& other) noexcept = default;

	CBigInteger operator/(const int divisor) const;

	CBigInteger operator^(const CBigInteger& rhs) const
	{
		CBigInteger<NUM_BYTES, ALLOC> result = *this;
		for (size_t i = 0; i < NUM_BYTES; i++)
		{
			result[i] ^= rhs[i];
		}

		return result;
	}

	unsigned char& operator[] (const size_t x) { return m_data[x]; }
	const unsigned char& operator[] (const size_t x) const { return m_data[x]; }

	inline bool operator<(const CBigInteger& rhs) const
	{
		if (this == &rhs)
		{
			return false;
		}

		auto rhsIter = rhs.m_data.cbegin();
		for (auto iter = this->m_data.cbegin(); iter != this->m_data.cend(); iter++)
		{
			if (*rhsIter != *iter)
			{
				return *iter < *rhsIter;
			}

			rhsIter++;
		}

		return false;
	}

	inline bool operator>(const CBigInteger& rhs) const
	{
		return rhs < *this;
	}

	inline bool operator==(const CBigInteger& rhs) const
	{
		if (this == &rhs)
		{
			return true;
		}

		for (size_t i = 0; i < NUM_BYTES; i++)
		{
			if (this->m_data[i] != rhs.m_data[i])
			{
				return false;
			}
		}

		return true;
	}

	inline bool operator!=(const CBigInteger& rhs) const
	{
		return !(*this == rhs);
	}

	inline bool operator<=(const CBigInteger& rhs) const
	{
		return *this < rhs || *this == rhs;
	}

	inline bool operator>=(const CBigInteger& rhs) const
	{
		return *this > rhs || *this == rhs;
	}

	inline CBigInteger operator^=(const CBigInteger& rhs)
	{
		*this = *this ^ rhs;

		return *this;
	}

private:
	std::vector<unsigned char, ALLOC> m_data;
};

static unsigned char FromHexChar(const char value)
{
	if (value <= '9' && value >= 0)
	{
		return (unsigned char)(value - '0');
	}

	if (value >= 'a')
	{
		return (unsigned char)(10 + value - 'a');
	}

	return (unsigned char)(10 + value - 'A');
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::FromHex(const std::string& hex)
{
	// TODO: Verify input size
	size_t index = 0;
	if (hex[0] == '0' && hex[1] == 'x')
	{
		index = 2;
	}

	std::string hexNoSpaces = "";
	for (size_t i = index; i < hex.length(); i++)
	{
		if (hex[i] != ' ')
		{
			hexNoSpaces += hex[i];
		}
	}

	std::vector<unsigned char, ALLOC> data(NUM_BYTES);
	for (size_t i = 0; i < hexNoSpaces.length(); i += 2)
	{
		data[i / 2] = (FromHexChar(hexNoSpaces[i]) * 16 + FromHexChar(hexNoSpaces[i + 1]));
	}

	return CBigInteger<NUM_BYTES, ALLOC>(&data[0]);
}

#ifdef INCLUDE_TEST_MATH

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator/(const int divisor) const
{
	std::vector<unsigned char, ALLOC> quotient(NUM_BYTES);

	int remainder = 0;
	for (int i = 0; i < NUM_BYTES; i++)
	{
		remainder = remainder * 256 + m_data[i];
		quotient[i] = (unsigned char)(remainder / divisor);
		remainder -= quotient[i] * divisor;
	}

	return CBigInteger<NUM_BYTES, ALLOC>(&quotient[0]);
}

#endif