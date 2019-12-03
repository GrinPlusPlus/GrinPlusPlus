#include "LoggerImpl.h"
#include "ThreadManagerImpl.h"

#include <Infrastructure/Logger.h>
#include <Common/Util/FileUtil.h>
#include <fstream>

Logger& Logger::GetInstance()
{
	static Logger instance;
	return instance;
}

void Logger::StartLogger(const std::string& logDirectory, const spdlog::level::level_enum& logLevel)
{
	FileUtil::CreateDirectories(logDirectory);

	{
		const std::string logPath = logDirectory + "Node.log";
		auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(StringUtil::ToWide(logPath), 10 * 1024 * 1024, 10);
		m_pNodeLogger = spdlog::create_async("NODE", sink, 32768, spdlog::async_overflow_policy::block_retry, nullptr, std::chrono::seconds(5));
		spdlog::set_pattern("[%D %X.%e%z] [%l] %v");
		if (m_pNodeLogger != nullptr)
		{
			m_pNodeLogger->set_level(logLevel);
		}
	}
	{
		const std::string logPath = logDirectory + "Wallet.log";
		auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(StringUtil::ToWide(logPath), 10 * 1024 * 1024, 10);
		m_pWalletLogger = spdlog::create_async("WALLET", sink, 8192, spdlog::async_overflow_policy::block_retry, nullptr, std::chrono::seconds(5));
		spdlog::set_pattern("[%D %X.%e%z] [%l] %v");
		if (m_pWalletLogger != nullptr)
		{
			m_pWalletLogger->set_level(logLevel);
		}
	}
}

void Logger::Log(const LoggerAPI::LogFile file, const spdlog::level::level_enum logLevel, const std::string& eventText)
{
	auto pLogger = GetLogger(file);
	if (pLogger != nullptr)
	{
		std::string eventTextClean = eventText;
		size_t newlinePos = eventTextClean.find("\n");
		while (newlinePos != std::string::npos)
		{
			if (eventTextClean.size() > newlinePos + 2)
			{
				eventTextClean.erase(newlinePos, 2);
			}
			else
			{
				eventTextClean.erase(newlinePos, 1);
			}

			newlinePos = eventTextClean.find("\n");
		}

		const std::string threadName = ThreadManager::GetInstance().GetCurrentThreadName();
		if (!threadName.empty())
		{
			eventTextClean = threadName + " " + eventTextClean;
		}

		pLogger->log(logLevel, eventTextClean);
	}
}

void Logger::Flush()
{
	if (m_pNodeLogger != nullptr)
	{
		m_pNodeLogger->flush();
	}

	if (m_pWalletLogger != nullptr)
	{
		m_pWalletLogger->flush();
	}
}

std::shared_ptr<spdlog::logger> Logger::GetLogger(const LoggerAPI::LogFile file)
{
	if (file == LoggerAPI::LogFile::WALLET)
	{
		return m_pWalletLogger;
	}
	else
	{
		return m_pNodeLogger;
	}
}

namespace LoggerAPI
{
	LOGGER_API void Initialize(const std::string& logDirectory, const std::string& logLevel)
	{
		spdlog::level::level_enum logLevelEnum = spdlog::level::level_enum::debug;
		if (logLevel == "TRACE")
		{
			logLevelEnum = spdlog::level::level_enum::trace;
		}
		else if (logLevel == "DEBUG")
		{
			logLevelEnum = spdlog::level::level_enum::debug;
		}
		else if (logLevel == "INFO")
		{
			logLevelEnum = spdlog::level::level_enum::info;
		}
		else if (logLevel == "WARN")
		{
			logLevelEnum = spdlog::level::level_enum::warn;
		}
		else if (logLevel == "ERROR")
		{
			logLevelEnum = spdlog::level::level_enum::err;
		}

		Logger::GetInstance().StartLogger(logDirectory, logLevelEnum);
	}

	LOGGER_API void LogTrace(const std::string& message)
	{
		Logger::GetInstance().Log(LogFile::NODE, spdlog::level::level_enum::trace, message);
	}

	LOGGER_API void LogDebug(const std::string& message)
	{
		Logger::GetInstance().Log(LogFile::NODE, spdlog::level::level_enum::debug, message);
	}

	LOGGER_API void LogInfo(const std::string& message)
	{
		Logger::GetInstance().Log(LogFile::NODE, spdlog::level::level_enum::info, message);
	}

	LOGGER_API void LogWarning(const std::string& message)
	{
		Logger::GetInstance().Log(LogFile::NODE, spdlog::level::level_enum::warn, message);
	}

	LOGGER_API void LogError(const std::string& message)
	{
		Logger::GetInstance().Log(LogFile::NODE, spdlog::level::level_enum::err, message);
	}

	LOGGER_API void Flush()
	{
		Logger::GetInstance().Flush();
	}


	LOGGER_API void LogTrace(const LogFile file, const std::string& function, const size_t line, const std::string& message)
	{
		const std::string formatted = function + "(" + std::to_string(line) + ") - " + message;
		Logger::GetInstance().Log(file, spdlog::level::level_enum::trace, formatted);
	}

	LOGGER_API void LogDebug(const LogFile file, const std::string& function, const size_t line, const std::string& message)
	{
		const std::string formatted = function + "(" + std::to_string(line) + ") - " + message;
		Logger::GetInstance().Log(file, spdlog::level::level_enum::debug, formatted);
	}

	LOGGER_API void LogInfo(const LogFile file, const std::string& function, const size_t line, const std::string& message)
	{
		const std::string formatted = function + "(" + std::to_string(line) + ") - " + message;
		Logger::GetInstance().Log(file, spdlog::level::level_enum::info, formatted);
	}

	LOGGER_API void LogWarning(const LogFile file, const std::string& function, const size_t line, const std::string& message)
	{
		const std::string formatted = function + "(" + std::to_string(line) + ") - " + message;
		Logger::GetInstance().Log(file, spdlog::level::level_enum::warn, formatted);
	}

	LOGGER_API void LogError(const LogFile file, const std::string& function, const size_t line, const std::string& message)
	{
		const std::string formatted = function + "(" + std::to_string(line) + ") - " + message;
		Logger::GetInstance().Log(file, spdlog::level::level_enum::err, formatted);
	}
}