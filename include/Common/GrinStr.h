#pragma once

#include <string>
#include <vector>
#include <filesystem.h>

#ifdef _WIN32
	#define GRIN_WIDE_STR std::wstring
#else
	#define GRIN_WIDE_STR std::string
#endif

class GrinStr : public std::string
{
public:
	GrinStr(const GrinStr& str) = default;
	GrinStr(GrinStr&& str) = default;
	GrinStr(const std::string& str) : std::string(str) { }
	GrinStr(std::string&& str) : std::string(std::move(str)) { }
	GrinStr(const char* str) : std::string(str) { }
	GrinStr(const const_iterator& begin, const const_iterator& end) : std::string(begin, end) { }

	GrinStr& operator=(const GrinStr& str) noexcept
	{
		assign(str);
		return *this;
	}
	GrinStr& operator=(const std::string& str) noexcept
	{
		assign(str);
		return *this;
	}

	std::vector<GrinStr> Split(const GrinStr& delimiter) const;

	bool StartsWith(const std::string& beginning) const noexcept;
	bool EndsWith(const std::string& ending) const noexcept;

	GrinStr Trim() const;
	GrinStr ToLower() const;
	GRIN_WIDE_STR ToWide() const noexcept;
	fs::path ToPath() const noexcept;
};