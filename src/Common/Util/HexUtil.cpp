#include <Common/Util/HexUtil.h>
#include <Common/GrinStr.h>
#include <Common/Logger.h>

#include <string>
#include <cppcodec/hex_lower.hpp>

bool HexUtil::IsValidHex(const std::string& data) noexcept
{
	if (data.size() > 2 && data.compare(0, 2, "0x") == 0) {
		return data.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
	} else {
		return data.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
	}
}

std::vector<uint8_t> HexUtil::FromHex(const std::string& hex)
{
	GrinStr hex_clean = GrinStr(hex).ToLower().Trim();
	if (hex_clean.StartsWith("0x")) {
		hex_clean = hex_clean.substr(2);
	}

	if (!IsValidHex(hex_clean)) {
		LOG_ERROR_F("Hex invalid: {}", hex_clean);
		throw std::exception();
	}

	std::vector<uint8_t> ret;
	cppcodec::hex_lower::decode(ret, hex_clean);
	return ret;
}

std::string HexUtil::ConvertToHex(const std::vector<uint8_t>& data)
{
	return cppcodec::hex_lower::encode(data.data(), data.size());
}

std::string HexUtil::ConvertToHex(const std::vector<uint8_t>& data, const size_t numBytes)
{
	return cppcodec::hex_lower::encode(data.data(), numBytes);
}