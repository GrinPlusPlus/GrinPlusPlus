#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <ios>
#include <iomanip>
#include <Hash.h>

namespace HexUtil
{
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

	static std::string ConvertHash(const Hash& hash)
	{
		const std::vector<unsigned char>& bytes = hash.GetData();
		const std::vector<unsigned char> firstFourBytes = std::vector<unsigned char>(bytes.cbegin(), bytes.cbegin() + 4);
		
		return ConvertToHex(firstFourBytes, false, false);
	}
}