#pragma once

#include "spdlog/spdlog.h"
#include <Infrastructure/Logger.h>

#include <string>
#include <memory>

class Logger
{
public:
	static Logger& GetInstance();

	void StartLogger(
		const fs::path& logDirectory,
		const spdlog::level::level_enum& logLevel
	);
	void Log(const LoggerAPI::LogFile file, const spdlog::level::level_enum logLevel, const std::string& eventText);
	void Flush();

private:
	Logger() = default;

	std::shared_ptr<spdlog::logger> GetLogger(const LoggerAPI::LogFile file);

	std::shared_ptr<spdlog::logger> m_pNodeLogger;
	std::shared_ptr<spdlog::logger> m_pWalletLogger;
};