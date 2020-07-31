#include <Common/GrinStr.h>
#include <Common/Util/StringUtil.h>

GrinStr GrinStr::Trim() const
{
	return StringUtil::Trim(*this);
}

GrinStr GrinStr::ToLower() const
{
	return StringUtil::ToLower(*this);
}

std::vector<GrinStr> GrinStr::Split(const GrinStr& delimiter) const
{
	std::vector<std::string> parts = StringUtil::Split(*this, delimiter);
	std::vector<GrinStr> converted;
	std::transform(
		parts.cbegin(), parts.cend(),
		std::back_inserter(converted),
		[](const std::string& str) { return GrinStr(str); }
	);

	return converted;
}

bool GrinStr::StartsWith(const std::string& beginning) const noexcept
{
	return StringUtil::StartsWith(*this, beginning);
}

bool GrinStr::EndsWith(const std::string& ending) const noexcept
{
	return StringUtil::EndsWith(*this, ending);
}

GRIN_WIDE_STR GrinStr::ToWide() const noexcept
{
	return StringUtil::ToWide(*this);
}

fs::path GrinStr::ToPath() const noexcept
{
	return fs::path(ToWide());
}