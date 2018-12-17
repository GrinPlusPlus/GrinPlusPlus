#include "LoggerImpl.h"
#include "ThreadManagerImpl.h"

#include <Infrastructure/Logger.h>
#include <fstream>
#include <filesystem>

Logger& Logger::GetInstance()
{
	static Logger instance;
	return instance;
}

Logger::Logger()
{
	const std::filesystem::path currentPath = std::filesystem::current_path();

	const std::string logPath = currentPath.string() + "/MimbleWimble.log"; // TODO: Create if path doesn't exist

	auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath, 1024 * 1024, 5);
	m_logger = spdlog::create_async("LOGGER", sink, 8192, spdlog::async_overflow_policy::block_retry, nullptr, std::chrono::seconds(2));
	if (m_logger != nullptr)
	{
		m_logger->set_level(spdlog::level::level_enum::debug); // TODO: Load from config.
	}
}

void Logger::Log(const spdlog::level::level_enum logLevel, const std::string& eventText)
{
	if (m_logger != nullptr)
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

		m_logger->log(logLevel, eventTextClean);
	}
}

void Logger::Flush()
{
	if (m_logger != nullptr)
	{
		m_logger->flush();
	}
}

namespace LoggerAPI
{
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