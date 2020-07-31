#pragma once

#include <string>
#include <cstdint>
#include <vector>

class HexUtil
{
public:
	static bool IsValidHex(const std::string& data) noexcept;
	static std::vector<uint8_t> FromHex(const std::string& hex);

	static std::string ConvertToHex(const std::vector<uint8_t>& data);
	static std::string ConvertToHex(const std::vector<uint8_t>& data, const size_t numBytes);
};