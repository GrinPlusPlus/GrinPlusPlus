#pragma once

#include <exception>

class CryptoException : public std::exception
{
private:
	struct ExceptionData
	{
		char const* m_pMessage;
	};

	ExceptionData m_data;

public:
	explicit CryptoException() noexcept
		: m_data()
	{
		m_data.m_pMessage = nullptr;
	}

	explicit CryptoException(char const* const pMessage) noexcept
		: m_data()
	{
		m_data.m_pMessage = pMessage;
	}

	explicit CryptoException(const std::string& message) noexcept
		: m_data()
	{
		m_data.m_pMessage = message.c_str();
	}

	const char* what() const throw()
	{
		return m_data.m_pMessage != nullptr ? m_data.m_pMessage : "An error occurred while performing a cryptographic operation.";
	}
};