#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ios>
#include <iomanip>
#include <Crypto/Hash.h>
#include <Core/Serialization/EndianHelper.h>
#include <cppcodec/hex_lower.hpp>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

namespace HexUtil
{
	static bool IsValidHex(const std::string& data)
	{
		if (data.compare(0, 2, "0x") == 0 && data.size() > 2)
		{
			return data.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
		}
		else
		{
			return data.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
		}
	}

	template<class ALLOC = std::allocator<unsigned char>>
	static std::vector<unsigned char, ALLOC> FromHex(const std::string& hex)
	{
		size_t index = 0;
		if (hex.size() >= 2)
		{
			if (hex[0] == '0' && hex[1] == 'x')
			{
				index = 2;
			}
		}

		std::string hexNoSpaces = "";
		for (size_t i = index; i < hex.length(); i++)
		{
			if (hex[i] != ' ')
			{
				hexNoSpaces += hex[i];
			}
		}

		if (!IsValidHex(hexNoSpaces))
		{
			LOG_ERROR_F("Hex invalid: {}", hexNoSpaces);
			throw std::exception();
		}

		std::vector<unsigned char, ALLOC> ret;
		cppcodec::hex_lower::decode(ret, hexNoSpaces);
		return ret;
	}

	static std::string ConvertToHex(const std::vector<unsigned char>& data)
	{
		return cppcodec::hex_lower::encode(data.data(), data.size());
	}

	static std::string ConvertToHex(const std::vector<unsigned char>& data, const size_t numBytes)
	{
		return cppcodec::hex_lower::encode(data.data(), numBytes);
	}

	static std::string ConvertToHex(const uint16_t value)
	{
		const uint16_t bigEndian = EndianHelper::GetBigEndian16(value);

		std::vector<unsigned char> bytes(2);
		memcpy(bytes.data(), (unsigned char*)&bigEndian, 2);

		std::string hex = ConvertToHex(bytes);
		const size_t firstNonZero = hex.find_first_not_of('0');
		hex.erase(0, (std::min)(firstNonZero, hex.size() - 1));

		return hex;
	}

	static std::string ShortHash(const Hash& hash)
	{
		return ConvertToHex(hash.GetData(), 6);
	}
}