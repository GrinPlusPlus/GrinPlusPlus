#pragma once

#include <string>
#include <stdint.h>

class TimeUtil
{
public:
	static std::string FormatLocal(const int64_t timestamp, const std::string format = "%Y-%m-%d %H:%M:%S%z")
	{
		std::time_t time(timestamp);
		std::tm tm;
		localtime_s(&tm, &time);
		char buffer[32];
		std::strftime(buffer, 32, format.c_str(), &tm);
		return std::string(buffer);
	}

	static std::string FormatUTC(const int64_t timestamp, const std::string format = "%Y-%m-%d %H:%M:%S UTC")
	{
		std::time_t time(timestamp);
		std::tm tm;
		localtime_s(&tm, &time);
		char buffer[32];
		std::strftime(buffer, 32, format.c_str(), &tm);
		return std::string(buffer);
	}
};