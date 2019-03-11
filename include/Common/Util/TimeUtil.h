#pragma once

#include <string>
#include <chrono>
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

	static int64_t ToInt64(const std::chrono::system_clock::time_point& timePoint)
	{
		return timePoint.time_since_epoch().count();
	}

	static std::chrono::system_clock::time_point ToTimePoint(const int64_t millisSinceEpoch)
	{
		return std::chrono::system_clock::time_point(std::chrono::milliseconds(millisSinceEpoch));
	}
};