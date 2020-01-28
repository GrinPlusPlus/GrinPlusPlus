#pragma once

#include <stdexcept>
#include <string>

#define TOR_EXCEPTION(message) TorException(message, __func__)

class TorException : public std::logic_error
{
public:
	TorException(const std::string& message, const std::string& function)
		: std::logic_error(message)
	{
		m_message = message;
		m_what = message + ": " + function;
	}

	const std::string& GetMsg() const
	{
		return m_message;
	}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

private:
	std::string m_what;
	std::string m_message;
};