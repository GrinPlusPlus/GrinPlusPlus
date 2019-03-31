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
	LOGGER_API void Flush();

	// TODO: void LogConsole(const std::string& message);
}