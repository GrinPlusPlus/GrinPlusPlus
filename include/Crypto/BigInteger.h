#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <stdint.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>

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

	inline const std::vector<unsigned char, ALLOC>& GetData() const
	{
		return m_data;
	}

	static CBigInteger<NUM_BYTES, ALLOC> ValueOf(const unsigned char value);
	static CBigInteger<NUM_BYTES, ALLOC> FromHex(const std::string& hex);
	static CBigInteger<NUM_BYTES, ALLOC> GetMaximumValue();

	inline size_t size() const { return NUM_BYTES; }
	inline const unsigned char* data() const { return m_data.data(); }

	const unsigned char* ToCharArray() const { return &m_data[0]; }
	std::string ToHex() const;

	CBigInteger<NUM_BYTES, ALLOC>& ReverseByteOrder();

	CBigInteger addMod(const CBigInteger& addend, const CBigInteger& mod) const;

	//
	// Operators
	//
	CBigInteger& operator=(const CBigInteger& other) = default;
	CBigInteger& operator=(CBigInteger&& other) noexcept = default;

	CBigInteger operator+(const CBigInteger& addend) const;
	CBigInteger operator-(const CBigInteger& amount) const;

	CBigInteger operator*(const int multiplier) const;
	CBigInteger operator*(const CBigInteger& A) const;

	CBigInteger operator/(const int divisor) const;
	CBigInteger operator/(const CBigInteger& divisor) const;

	CBigInteger operator^(const CBigInteger& xor) const;

	int operator%(const int modulo) const;
	CBigInteger operator%(const CBigInteger& modulo) const;

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

	inline CBigInteger operator+=(const CBigInteger& rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	inline CBigInteger operator-=(const CBigInteger& rhs)
	{
		*this = *this - rhs;
		return *this;
	}

	inline CBigInteger operator*=(const CBigInteger& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	inline CBigInteger operator/=(const CBigInteger& rhs)
	{
		*this = *this / rhs;
		return *this;
	}

	inline CBigInteger operator^=(const CBigInteger& rhs)
	{
		*this = *this ^ rhs;

		return *this;
	}

private:
	std::vector<unsigned char, ALLOC> m_data;
};

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::ValueOf(const unsigned char value)
{
	std::vector<unsigned char, ALLOC> data(NUM_BYTES);
	data[NUM_BYTES - 1] = value;
	return CBigInteger<NUM_BYTES, ALLOC>(&data[0]);
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::GetMaximumValue()
{
	std::vector<unsigned char, ALLOC> data(NUM_BYTES);
	for (int i = 0; i < NUM_BYTES; i++)
	{
		data[i] = 0xFF;
	}

	return CBigInteger<NUM_BYTES, ALLOC>(&data[0]);
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
std::string CBigInteger<NUM_BYTES, ALLOC>::ToHex() const
{
	std::ostringstream stream;
	for (const unsigned char byte : m_data)
	{
		stream << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int)byte;
	}

	return stream.str();
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC>& CBigInteger<NUM_BYTES, ALLOC>::ReverseByteOrder()
{
	std::reverse(m_data.begin(), m_data.end());
	return *this;
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::addMod(const CBigInteger<NUM_BYTES, ALLOC>& addend, const CBigInteger<NUM_BYTES, ALLOC>& mod) const
{
	return *this + addend; // TODO: Handle mod
	//std::vector<unsigned char, ALLOC> zeroVector(NUM_BYTES, 0);
	//std::vector<unsigned char, ALLOC> totalSum(NUM_BYTES);

	//int carry = 0;

	//for (int i = NUM_BYTES - 1; i >= 1; i--)
	//{
	//	int digit1 = m_data[i];
	//	int digit2 = addend.GetData()[i];

	//	int sum = digit1 + digit2 + carry;

	//	if (sum > 255)
	//	{
	//		carry = 1;
	//		sum -= 256;
	//	}
	//	else
	//	{
	//		carry = 0;
	//	}

	//	totalSum[i] = sum;
	//}

	//int digit1 = m_data[0];
	//int digit2 = addend.GetData()[0];
	//int sum = digit1 + digit2 + carry;

	//while (sum > 255)
	//{
	//	totalSum[0] = 255;
	//	int remainder = sum - 255;
	//	CBigInteger<NUM_BYTES, ALLOC> sum(&totalSum[0]);

	//	CBigInteger<NUM_BYTES, ALLOC> mod = sum % mod;
	//	totalSum = mod.GetData();

	//	sum = totalSum[0] +
	//}

	//return CBigInteger<NUM_BYTES, ALLOC>(&totalSum[0]);
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator*(const CBigInteger<NUM_BYTES, ALLOC>& A) const
{
	const CBigInteger<NUM_BYTES, ALLOC> ZERO = CBigInteger<NUM_BYTES, ALLOC>::ValueOf(0);
	if (A == ZERO)
	{
		return ZERO;
	}

	CBigInteger<NUM_BYTES, ALLOC> multiplier = CBigInteger<NUM_BYTES, ALLOC>::ValueOf(1);
	CBigInteger<NUM_BYTES, ALLOC> nextMultiplier = CBigInteger<NUM_BYTES, ALLOC>::ValueOf(2);
	CBigInteger<NUM_BYTES, ALLOC> product = *this;

	while (nextMultiplier <= A)
	{
		multiplier = nextMultiplier;

		Double(product);
		Double(nextMultiplier);
	}

	CBigInteger<NUM_BYTES, ALLOC> remaining = A - multiplier;

	if (remaining > ZERO)
	{
		product = product + (*this * remaining);
	}

	return product;


	//// TODO: This math is all wrong
	//std::vector<unsigned char, ALLOC> tempNUM_BYTES;

	//int result = 0;
	//int k = 0;
	//for (int i = 0; i < A.NUM_BYTES; i++)
	//{
	//	k = i;
	//	int carry = 0;

	//	for (int j = 0; j < this->NUM_BYTES; j++)
	//	{
	//		result = A.m_data[i] * this->m_data[j] + temp[k];
	//		temp[k] = (result + carry) % 256;
	//		carry = (result + carry) / 256;
	//		k++;
	//	}

	//	if (carry != 0)
	//	{
	//		temp[k] = temp[k] + carry;
	//		k++;
	//	}
	//}

	//CBigInteger<NUM_BYTES, ALLOC> product(&temp[0]);
	//return product;
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator+(const CBigInteger<NUM_BYTES, ALLOC>& addend) const
{
	std::vector<unsigned char, ALLOC> totalSum(NUM_BYTES);

	int carry = 0;

	for (int i = NUM_BYTES - 1; i >= 0; i--)
	{
		int digit1 = m_data[i];
		int digit2 = addend.GetData()[i];

		int sum = digit1 + digit2 + carry;

		if (sum > 255)
		{
			carry = 1;
			sum -= 256;
		}
		else
		{
			carry = 0;
		}

		totalSum[i] = (unsigned char)sum;
	}

	return CBigInteger<NUM_BYTES, ALLOC>(&totalSum[0]);
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator-(const CBigInteger<NUM_BYTES, ALLOC>& amount) const
{
	std::vector<unsigned char, ALLOC> result(NUM_BYTES);

	int carry = 0;

	for (int i = NUM_BYTES - 1; i >= 0; i--)
	{
		int digit1 = m_data[i];
		int digit2 = amount.GetData()[i];

		int temp = digit1 - carry;
		carry = 0;

		if ((digit1 - carry) < digit2)
		{
			carry = 1;
			temp += 256;
		}

		result[i] = (unsigned char)(temp - digit2);
	}

	return CBigInteger<NUM_BYTES, ALLOC>(&result[0]);
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator*(const int multiplier) const
{
	CBigInteger temp(&m_data[0]);
	for (int i = 1; i < multiplier; i++)
	{
		temp = temp + *this;
	}

	return temp;
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator/(const int divisor) const
{
	std::vector<unsigned char, ALLOC> quotient(NUM_BYTES);

	int remainder = 0;
	for (int i = 0; i < NUM_BYTES; i++)
	{
		remainder = remainder * 256 + m_data[i];
		quotient[i] = remainder / divisor;
		remainder -= quotient[i] * divisor;
	}

	return CBigInteger<NUM_BYTES, ALLOC>(&quotient[0]);
}

template<size_t NUM_BYTES, class ALLOC>
static void Double(CBigInteger<NUM_BYTES, ALLOC>& number)
{
	// TODO: Handle overflow
	number = number + number;
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator/(const CBigInteger<NUM_BYTES, ALLOC>& divisor) const
{
	CBigInteger<NUM_BYTES, ALLOC> remaining = *this;

	const CBigInteger<NUM_BYTES, ALLOC> ZERO = CBigInteger<NUM_BYTES, ALLOC>::ValueOf(0);

	CBigInteger<NUM_BYTES, ALLOC> multiplier = CBigInteger<NUM_BYTES, ALLOC>::ValueOf(0);
	CBigInteger<NUM_BYTES, ALLOC> prevTotal = divisor;
	CBigInteger<NUM_BYTES, ALLOC> total = divisor;

	while (total <= remaining)
	{
		prevTotal = total;
		Double(total);

		if (multiplier == ZERO)
		{
			multiplier = CBigInteger<NUM_BYTES, ALLOC>::ValueOf(1);
		}
		else
		{
			Double(multiplier);
		}
	}

	total = prevTotal;

	CBigInteger<NUM_BYTES, ALLOC> quotient = multiplier;
	remaining  = remaining - total;
	
	if (remaining >= divisor)
	{
		quotient = quotient + remaining / divisor;
	}

	return quotient;
}

template<size_t NUM_BYTES, class ALLOC>
int CBigInteger<NUM_BYTES, ALLOC>::operator%(const int modulo) const
{
	CBigInteger<NUM_BYTES, ALLOC> quotient = *this / modulo;

	CBigInteger<NUM_BYTES, ALLOC> product = quotient * modulo;
	CBigInteger<NUM_BYTES, ALLOC> modResult = *this - product;

	return modResult.m_data[NUM_BYTES - 1];
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator%(const CBigInteger<NUM_BYTES, ALLOC>& modulo) const
{
	CBigInteger<NUM_BYTES, ALLOC> quotient = *this / modulo;

	CBigInteger<NUM_BYTES, ALLOC> product = quotient * modulo;
	CBigInteger<NUM_BYTES, ALLOC> modResult = *this - product;

	return modResult;
}

template<size_t NUM_BYTES, class ALLOC>
CBigInteger<NUM_BYTES, ALLOC> CBigInteger<NUM_BYTES, ALLOC>::operator^(const CBigInteger<NUM_BYTES, ALLOC>& xor) const
{
	CBigInteger<NUM_BYTES, ALLOC> result = *this;
	for (size_t i = 0; i < NUM_BYTES; i++)
	{
		result[i] ^= xor[i];
	}

	return result;
}