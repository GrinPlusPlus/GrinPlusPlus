#pragma once

#include <string>
#include <exception>
#include <Common/Util/StringUtil.h>

#define CRYPTO_EXCEPTION(msg) CryptoException(msg, __func__)
#define CRYPTO_EXCEPTION_F(msg, ...) CryptoException(StringUtil::Format(msg, __VA_ARGS__), __func__)

class CryptoException : public std::exception
{
private:
	std::string m_what;

public:
	explicit CryptoException() noexcept
	{
		m_what = "An error occurred while performing a cryptographic operation.";
	}

	explicit CryptoException(char const* const pMessage) noexcept
	{
		m_what = pMessage;
	}

	explicit CryptoException(const std::string& message) noexcept
	{
		m_what = message;
	}

	CryptoException(const std::string& message, const std::string& function) noexcept
	{
		m_what = function + ": " + message;
	}

	const char* what() const throw()
	{
		return m_what.c_str();
	}
};