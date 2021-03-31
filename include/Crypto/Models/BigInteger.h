#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Traits/Printable.h>
#include <Common/Util/HexUtil.h>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma warning(disable: 4505)

template<size_t NUM_BYTES, class ALLOC = std::allocator<unsigned char>>
class CBigInteger : public Traits::IPrintable
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
		: m_data(data.cbegin(), data.cbegin() + NUM_BYTES)
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

	CBigInteger(const CBigInteger& bigInteger) = default;
	CBigInteger(CBigInteger&& bigInteger) noexcept = default;

	//
	// Destructor
	//
	virtual ~CBigInteger() = default;

	void erase()
	{
		for (size_t i = 0; i < NUM_BYTES; i++)
		{
			m_data[i] = 0;
		}
	}

	const std::vector<unsigned char, ALLOC>& GetData() const
	{
		return m_data;
	}

	auto cbegin() const noexcept
	{
		return m_data.cbegin();
	}

	auto cend() const noexcept
	{
		return m_data.cend();
	}

	auto begin() noexcept
	{
		return m_data.begin();
	}

	auto end() noexcept
	{
		return m_data.end();
	}

	static CBigInteger<NUM_BYTES, ALLOC> ValueOf(const unsigned char value)
	{
		std::vector<unsigned char, ALLOC> data(NUM_BYTES);
		data[NUM_BYTES - 1] = value;
		return CBigInteger<NUM_BYTES, ALLOC>(std::move(data));
	}

	static CBigInteger<NUM_BYTES, ALLOC> FromHex(const std::string& hex)
	{
		std::vector<uint8_t> data = HexUtil::FromHex(hex);
		if (data.size() != NUM_BYTES)
		{
			throw std::exception();
		}

		return CBigInteger<NUM_BYTES, ALLOC>(data.data());
	}

	static CBigInteger<NUM_BYTES, ALLOC> GetMaximumValue()
	{
		std::vector<unsigned char, ALLOC> data(NUM_BYTES);
		for (int i = 0; i < NUM_BYTES; i++)
		{
			data[i] = 0xFF;
		}

		return CBigInteger<NUM_BYTES, ALLOC>(&data[0]);
	}

	size_t size() const { return NUM_BYTES; }
	unsigned char* data() { return m_data.data(); }
	const unsigned char* data() const { return m_data.data(); }

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
	std::string Format() const final { return ToHex(); }

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

	CBigInteger& operator++()
	{
		for (size_t i = NUM_BYTES - 1; i >= 0; i--)
		{
			if (m_data[i] == UINT8_MAX) {
				m_data[i] = 0;
			}
			else {
				++m_data[i];
				break;
			}
		}

		return *this;
	}

	CBigInteger operator++(int)
	{
		CBigInteger<NUM_BYTES, ALLOC> current = *this;
		for (size_t i = NUM_BYTES - 1; i >= 0; i--)
		{
			if (m_data[i] == UINT8_MAX) {
				m_data[i] = 0;
			}
			else {
				++m_data[i];
				break;
			}
		}

		return current;
	}

	unsigned char& operator[] (const size_t x) { return m_data[x]; }
	const unsigned char& operator[] (const size_t x) const { return m_data[x]; }

	bool operator<(const CBigInteger& rhs) const
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

	bool operator>(const CBigInteger& rhs) const
	{
		return rhs < *this;
	}

	bool operator==(const CBigInteger& rhs) const
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

	bool operator!=(const CBigInteger& rhs) const
	{
		return !(*this == rhs);
	}

	bool operator<=(const CBigInteger& rhs) const
	{
		return *this < rhs || *this == rhs;
	}

	bool operator>=(const CBigInteger& rhs) const
	{
		return *this > rhs || *this == rhs;
	}

	CBigInteger operator^=(const CBigInteger& rhs)
	{
		*this = *this ^ rhs;

		return *this;
	}

private:
	std::vector<unsigned char, ALLOC> m_data;
};

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
