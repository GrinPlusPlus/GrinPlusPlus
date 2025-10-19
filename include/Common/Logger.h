#pragma once

#include <Common/Util/StringUtil.h>
#include <string>

#define LOGGER_API

namespace LoggerAPI
{
	enum class LogFile
	{
		NODE,
		WALLET
	};

	enum class LogLevel
	{
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERR
	};

	LOGGER_API void Initialize(const fs::path& logDirectory, const std::string& logLevel);
	LOGGER_API void Shutdown();
	LOGGER_API void SetThreadName(const std::string& thread_name);
	LOGGER_API bool WillLog(const LogFile file, const LogLevel level);

	LOGGER_API void LogTrace(const std::string& message);
	LOGGER_API void LogDebug(const std::string& message);
	LOGGER_API void LogInfo(const std::string& message);
	LOGGER_API void LogWarning(const std::string& message);
	LOGGER_API void LogError(const std::string& message);
	LOGGER_API void Flush();

	LOGGER_API void Log(const LogFile file, const LogLevel level, const std::string& message);
	LOGGER_API void LogTrace(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogDebug(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogInfo(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogWarning(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogError(const LogFile file, const std::string& function, const size_t line, const std::string& message);
}

#define APPLOG_FMT(logFile, logLevel, fmt, ...) do { \
    if (LoggerAPI::WillLog(logFile, logLevel)) { \
        LoggerAPI::Log(logFile, logLevel, StringUtil::Format("{}({}) - " fmt, __func__, __LINE__, __VA_ARGS__)); \
    } \
} while (0)

#define APPLOG_MSG(logFile, logLevel, msg) do { \
    if (LoggerAPI::WillLog(logFile, logLevel)) { \
        LoggerAPI::Log(logFile, logLevel, StringUtil::Format("{}({}) - {}", __func__, __LINE__, msg)); \
    } \
} while (0)


// Argument-counter / dispatcher (works up to N=15; extend if you�re chatty)
#define APPLOG_INVOKE(macro, file, level, ...) macro(file, level, __VA_ARGS__)
#define APPLOG_CHOOSE(_1,_2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, NAME, ...) NAME
#define APPLOG(file, level, ...)                                          \
    APPLOG_INVOKE( APPLOG_CHOOSE(__VA_ARGS__, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, \
    APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_FMT, APPLOG_MSG), \
    file, level, __VA_ARGS__) 


// Node Logger
#define LOG_TRACE(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::WARN, __VA_ARGS__)
#define LOG_ERROR(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::ERR, __VA_ARGS__)

#define LOG_TRACE_F(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG_F(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO_F(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING_F(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::WARN, __VA_ARGS__)
#define LOG_ERROR_F(...) APPLOG(LoggerAPI::LogFile::NODE, LoggerAPI::LogLevel::ERR, __VA_ARGS__)

// Wallet Logger
#define WALLET_TRACE(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::TRACE, __VA_ARGS__)
#define WALLET_DEBUG(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::DEBUG, __VA_ARGS__)
#define WALLET_INFO(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::INFO, __VA_ARGS__)
#define WALLET_WARNING(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::WARN, __VA_ARGS__)
#define WALLET_ERROR(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::ERR, __VA_ARGS__)

#define WALLET_TRACE_F(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::TRACE, __VA_ARGS__)
#define WALLET_DEBUG_F(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::DEBUG, __VA_ARGS__)
#define WALLET_INFO_F(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::INFO, __VA_ARGS__)
#define WALLET_WARNING_F(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::WARN, __VA_ARGS__)
#define WALLET_ERROR_F(...) APPLOG(LoggerAPI::LogFile::WALLET, LoggerAPI::LogLevel::ERR, __VA_ARGS__)
