#pragma once

#include <stdexcept>
#include <string>

#define TOR_EXCEPTION(message) TorException(message, __FUNCTION__)

class TorException : public std::logic_error
{
public:
	TorException(const std::string& message, const std::string& function)
		: std::logic_error(message)
	{
		m_message = message + ": " + function;
	}

	virtual const char* what() const throw()
	{
		return m_message.c_str();
	}

private:
	std::string m_message;
};