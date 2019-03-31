#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <locale>

#pragma warning(disable : 4840)

namespace StringUtil
{
	template<typename ... Args>
	std::string Format(const std::string& format, Args ... args)
	{
		size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
	}

	inline bool StartsWith(const std::string& value, const std::string& beginning)
	{
		if (beginning.size() > value.size())
		{
			return false;
		}

		return std::equal(beginning.begin(), beginning.end(), value.begin());
	}

	inline bool EndsWith(const std::string& value, const std::string& ending)
	{
		if (ending.size() > value.size())
		{
			return false;
		}

		return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
	}

	static std::vector<std::string> Split(const std::string& str, const std::string& delimiter)
	{
		// Skip delimiters at beginning.
		std::string::size_type lastPos = str.find_first_not_of(delimiter, 0);

		// Find first non-delimiter.
		std::string::size_type pos = str.find_first_of(delimiter, lastPos);

		std::vector<std::string> tokens;
		while (std::string::npos != pos || std::string::npos != lastPos)
		{
			// Found a token, add it to the vector.
			tokens.push_back(str.substr(lastPos, pos - lastPos));

			// Skip delimiters.
			lastPos = str.find_first_not_of(delimiter, pos);

			// Find next non-delimiter.
			pos = str.find_first_of(delimiter, lastPos);
		}

		return tokens;
	}

	static std::string ToLower(const std::string& str)
	{
		std::locale loc;
		std::string output = "";

		for (auto elem : str)
		{
			output += std::tolower(elem, loc);
		}

		return output;
	}
}