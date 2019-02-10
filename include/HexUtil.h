#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <ios>
#include <iomanip>
#include <Hash.h>

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

	static std::string ConvertToHex(const std::vector<unsigned char>& data, const bool upperCase, const bool includePrefix)
	{
		std::ostringstream stream;
		for (const unsigned char byte : data)
		{
			stream << std::hex << std::setfill('0') << std::setw(2) << (upperCase ? std::uppercase : std::nouppercase) << (int)byte;
		}

		if (includePrefix)
		{
			return "0x" + stream.str();
		}

		return stream.str();
	}

	static std::string ConvertToHex(const std::vector<unsigned char>& data, const size_t numBytes)
	{
		const std::vector<unsigned char> firstNBytes = std::vector<unsigned char>(data.cbegin(), data.cbegin() + numBytes);

		return ConvertToHex(firstNBytes, false, false);
	}

	static std::string ConvertHash(const Hash& hash)
	{
		const std::vector<unsigned char>& bytes = hash.GetData();
		const std::vector<unsigned char> firstSixBytes = std::vector<unsigned char>(bytes.cbegin(), bytes.cbegin() + 6);
		
		return ConvertToHex(firstSixBytes, false, false);
	}
}