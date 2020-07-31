#pragma once

#include <Core/Traits/Printable.h>
#include <fmt/format.h>
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
	friend class GrinStr;

	template<typename ... Args>
	static std::string Format(const char* format, const Args& ... args)
	{
		return fmt::format(format, ConvertParam(args) ...);
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
			output += std::tolower(elem, loc);
		}

		return output;
	}

	static std::string ToUpper(const std::string& str)
	{
		std::locale loc;
		std::string output = "";

		for (char elem : str)
		{
			output += std::toupper(elem, loc);
		}

		return output;
	}

	static std::string ToUTF8(const std::wstring& wstr)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		return converter.to_bytes(wstr);
	}

#ifdef _WIN32
	static std::wstring ToWide(const std::string& str)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(str);
	}
#else
	static std::string ToWide(const std::string& str)
	{
		return str;
	}
#endif

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

	static std::string ConvertParam(const std::string& x)
	{
		return x;
	}

	static std::string ConvertParam(const char* x)
	{
		return std::string(x);
	}

	static std::string ConvertParam(const std::wstring& x)
	{
		return StringUtil::ToUTF8(x);
	}

	static std::string ConvertParam(const fs::path& path)
	{
		return path.u8string();
	}

	static std::string ConvertParam(const Traits::IPrintable& x)
	{
		return x.Format();
	}

	static std::string ConvertParam(const std::shared_ptr<const Traits::IPrintable>& x)
	{
		if (x == nullptr)
		{
			return "NULL";
		}

		return x->Format();
	}

	static std::string ConvertParam(const std::exception& e)
	{
		return std::string(e.what());
	}

	template <class T, typename SFINAE = std::enable_if_t<std::is_fundamental_v<T>>>
	static decltype(auto) ConvertParam(const T& x)
	{
		return x;
	}
};