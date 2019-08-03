#pragma once

#include <Common/ImportExport.h>
#include <string>

#ifdef MW_INFRASTRUCTURE
#define LOGGER_API EXPORT
#else
#define LOGGER_API IMPORT
#endif

namespace LoggerAPI
{
	LOGGER_API void Initialize(const std::string& directory, const std::string& logLevel);

	LOGGER_API void LogTrace(const std::string& message);
	LOGGER_API void LogDebug(const std::string& message);
	LOGGER_API void LogInfo(const std::string& message);
	LOGGER_API void LogWarning(const std::string& message);
	LOGGER_API void LogError(const std::string& message);
	// TODO: void LogConsole(const std::string& message);
	LOGGER_API void Flush();


	LOGGER_API void LogTrace(const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogDebug(const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogInfo(const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogWarning(const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogError(const std::string& function, const size_t line, const std::string& message);
}

// TODO: Wallet Logger?
#define LOG_TRACE(message) LoggerAPI::LogTrace(__FUNCTION__, __LINE__, message)
#define LOG_DEBUG(message) LoggerAPI::LogDebug(__FUNCTION__, __LINE__, message)
#define LOG_INFO(message) LoggerAPI::LogInfo(__FUNCTION__, __LINE__, message)
#define LOG_WARNING(message) LoggerAPI::LogWarning(__FUNCTION__, __LINE__, message)
#define LOG_ERROR(message) LoggerAPI::LogError(__FUNCTION__, __LINE__, message)