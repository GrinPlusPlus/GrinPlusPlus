#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ios>
#include <iomanip>
#include <Crypto/Hash.h>
#include <Core/Serialization/EndianHelper.h>

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

	static std::string ConvertToHex(const std::vector<unsigned char>& data)
	{
		std::ostringstream stream;
		for (const unsigned char byte : data)
		{
			stream << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int)byte;
		}

		return stream.str();
	}

	static std::string ConvertToHex(const std::vector<unsigned char>& data, const size_t numBytes)
	{
		const std::vector<unsigned char> firstNBytes = std::vector<unsigned char>(data.cbegin(), data.cbegin() + numBytes);

		return ConvertToHex(firstNBytes);
	}

	static std::string ConvertToHex(const uint16_t value, const bool abbreviate)
	{
		const uint16_t bigEndian = EndianHelper::GetBigEndian16(value);

		std::vector<unsigned char> bytes(2);
		memcpy(&bytes, (unsigned char*)&bigEndian, 2);

		std::string hex = ConvertToHex(bytes);
		const size_t firstNonZero = hex.find_first_not_of('0');
		hex.erase(0, std::min(firstNonZero, hex.size() - 1));

		return hex;
	}

	static std::string ConvertHash(const Hash& hash)
	{
		const std::vector<unsigned char>& bytes = hash.GetData();
		const std::vector<unsigned char> firstSixBytes = std::vector<unsigned char>(bytes.cbegin(), bytes.cbegin() + 6);
		
		return ConvertToHex(firstSixBytes);
	}
}