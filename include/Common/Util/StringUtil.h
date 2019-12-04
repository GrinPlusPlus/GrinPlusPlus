#pragma once

#include <Core/Traits/Printable.h>
#include <memory>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <filesystem.h>

#pragma warning(disable : 4840)

class StringUtil
{
public:
	template<typename ... Args>
	static std::string Format(const std::string& format, const Args& ... args)
	{
		return Format2(format, convert_for_snprintf(args) ...);
	}

	static bool StartsWith(const std::string& value, const std::string& beginning)
	{
		if (beginning.size() > value.size())
		{
			return false;
		}

		return std::equal(beginning.begin(), beginning.end(), value.begin());
	}

	static bool EndsWith(const std::string& value, const std::string& ending)
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

		for (char elem : str)
		{
			output += static_cast<char>(std::tolower(static_cast<unsigned char>(elem), loc));
		}

		return output;
	}

	static std::string ToUTF8(const std::wstring& wstr)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		return converter.to_bytes(wstr);
	}

	static std::wstring ToWide(const std::string& str)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(str);
	}

	static inline std::string Trim(const std::string& s)
	{
		std::string copy = s;
		ltrim(copy);
		rtrim(copy);
		return copy;
	}

private:
	// trim from start (in place)
	static inline void ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
			}));
	}

	// trim from end (in place)
	static inline void rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch) && ch != '\r' && ch != '\n';
			}).base(), s.end());
	}

	template<typename ... Args>
	static std::string Format2(const std::string& format, const Args& ... args)
	{
		size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
	}

	static decltype(auto) convert_for_snprintf(const std::string& x)
	{
		return x.c_str();
	}

	static decltype(auto) convert_for_snprintf(const std::wstring& x)
	{
		return x.c_str();
	}

	static decltype(auto) convert_for_snprintf(const fs::path& path)
	{
		return path.wstring().c_str();
	}

	template <class T>
	static typename std::enable_if<std::is_base_of<Traits::IPrintable, T>::value, const char*>::type convert_for_snprintf(const T& x)
	{
		return x.Format().c_str();
	}


	template <class T>
	static typename std::enable_if<!std::is_base_of<Traits::IPrintable, T>::value, T>::type convert_for_snprintf(const T& x)
	{
		return x;
	}
};
