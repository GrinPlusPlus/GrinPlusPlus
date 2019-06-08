#include "LoggerImpl.h"
#include "ThreadManagerImpl.h"

#include <Infrastructure/Logger.h>
#include <fstream>

Logger& Logger::GetInstance()
{
	static Logger instance;
	return instance;
}

Logger::Logger()
{
}

void Logger::StartLogger(const std::string& directory, const spdlog::level::level_enum& logLevel)
{
	const std::string logPath = directory + "GrinPlusPlus.log";

	auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath, 1024 * 1024, 5);
	m_pLogger = spdlog::create_async("LOGGER", sink, 8192, spdlog::async_overflow_policy::block_retry, nullptr, std::chrono::seconds(5));
	if (m_pLogger != nullptr)
	{
		m_pLogger->set_level(logLevel);
	}
}

void Logger::Log(const spdlog::level::level_enum logLevel, const std::string& eventText)
{
	if (m_pLogger != nullptr)
	{
		std::string eventTextClean = eventText;
		while (eventTextClean.find("\n") != std::string::npos)
		{
			eventTextClean.erase(eventTextClean.find("\n"), 2);
		}

		const std::string threadName = ThreadManager::GetInstance().GetCurrentThreadName();
		if (!threadName.empty())
		{
			eventTextClean = threadName + " => " + eventTextClean;
		}

		m_pLogger->log(logLevel, eventTextClean);
	}
}

void Logger::Flush()
{
	if (m_pLogger != nullptr)
	{
		m_pLogger->flush();
	}
}

namespace LoggerAPI
{
	LOGGER_API void Initialize(const std::string& directory, const std::string& logLevel)
	{
		spdlog::level::level_enum logLevelEnum = spdlog::level::level_enum::debug;
		if (logLevel == "TRACE")
		{
			logLevelEnum = spdlog::level::level_enum::trace;
		}

		// TODO: Finish this.
		Logger::GetInstance().StartLogger(directory, logLevelEnum);
	}

	LOGGER_API void LogTrace(const std::string& message)
	{
		Logger::GetInstance().Log(spdlog::level::level_enum::trace, message);
	}

	LOGGER_API void LogDebug(const std::string& message)
	{
		Logger::GetInstance().Log(spdlog::level::level_enum::debug, message);
	}

	LOGGER_API void LogInfo(const std::string& message)
	{
		Logger::GetInstance().Log(spdlog::level::level_enum::info, message);
	}

	LOGGER_API void LogWarning(const std::string& message)
	{
		Logger::GetInstance().Log(spdlog::level::level_enum::warn, message);
	}

	LOGGER_API void LogError(const std::string& message)
	{
		Logger::GetInstance().Log(spdlog::level::level_enum::err, message);
	}

	LOGGER_API void Flush()
	{
		Logger::GetInstance().Flush();
	}
}