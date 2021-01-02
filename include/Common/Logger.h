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

	LOGGER_API void Initialize(const fs::path& logDirectory, const std::string& logLevel);
	LOGGER_API void Shutdown();
	LOGGER_API void SetThreadName(const std::string& thread_name);

	LOGGER_API void LogTrace(const std::string& message);
	LOGGER_API void LogDebug(const std::string& message);
	LOGGER_API void LogInfo(const std::string& message);
	LOGGER_API void LogWarning(const std::string& message);
	LOGGER_API void LogError(const std::string& message);
	LOGGER_API void Flush();


	LOGGER_API void LogTrace(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogDebug(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogInfo(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogWarning(const LogFile file, const std::string& function, const size_t line, const std::string& message);
	LOGGER_API void LogError(const LogFile file, const std::string& function, const size_t line, const std::string& message);
}

// Node Logger
#define LOG_TRACE(message) LoggerAPI::LogTrace(LoggerAPI::LogFile::NODE, __func__, __LINE__, message)
#define LOG_DEBUG(message) LoggerAPI::LogDebug(LoggerAPI::LogFile::NODE, __func__, __LINE__, message)
#define LOG_INFO(message) LoggerAPI::LogInfo(LoggerAPI::LogFile::NODE, __func__, __LINE__, message)
#define LOG_WARNING(message) LoggerAPI::LogWarning(LoggerAPI::LogFile::NODE, __func__, __LINE__, message)
#define LOG_ERROR(message) LoggerAPI::LogError(LoggerAPI::LogFile::NODE, __func__, __LINE__, message)

#define LOG_TRACE_F(message, ...) LoggerAPI::LogTrace(LoggerAPI::LogFile::NODE, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define LOG_DEBUG_F(message, ...) LoggerAPI::LogDebug(LoggerAPI::LogFile::NODE, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define LOG_INFO_F(message, ...) LoggerAPI::LogInfo(LoggerAPI::LogFile::NODE, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define LOG_WARNING_F(message, ...) LoggerAPI::LogWarning(LoggerAPI::LogFile::NODE, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define LOG_ERROR_F(message, ...) LoggerAPI::LogError(LoggerAPI::LogFile::NODE, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))

// Wallet Logger
#define WALLET_TRACE(message) LoggerAPI::LogTrace(LoggerAPI::LogFile::WALLET, __func__, __LINE__, message)
#define WALLET_DEBUG(message) LoggerAPI::LogDebug(LoggerAPI::LogFile::WALLET, __func__, __LINE__, message)
#define WALLET_INFO(message) LoggerAPI::LogInfo(LoggerAPI::LogFile::WALLET, __func__, __LINE__, message)
#define WALLET_WARNING(message) LoggerAPI::LogWarning(LoggerAPI::LogFile::WALLET, __func__, __LINE__, message)
#define WALLET_ERROR(message) LoggerAPI::LogError(LoggerAPI::LogFile::WALLET, __func__, __LINE__, message)

#define WALLET_TRACE_F(message, ...) LoggerAPI::LogTrace(LoggerAPI::LogFile::WALLET, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define WALLET_DEBUG_F(message, ...) LoggerAPI::LogDebug(LoggerAPI::LogFile::WALLET, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define WALLET_INFO_F(message, ...) LoggerAPI::LogInfo(LoggerAPI::LogFile::WALLET, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define WALLET_WARNING_F(message, ...) LoggerAPI::LogWarning(LoggerAPI::LogFile::WALLET, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define WALLET_ERROR_F(message, ...) LoggerAPI::LogError(LoggerAPI::LogFile::WALLET, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))