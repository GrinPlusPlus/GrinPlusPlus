#pragma once

#include <exception>
#include <string>

#define SOCKS_EXCEPTION(msg) SOCKSException(__func__, msg)

class SOCKSException : public std::exception
{
public:
	SOCKSException(const std::string& function, const std::string& message)
		: std::exception()
	{
		m_message = message;
		m_what = function + " - " + message;
	}

	SOCKSException() : std::exception()
	{
		m_message = "SOCKS Exception";
		m_what = m_message;
	}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

private:
	std::string m_message;
	std::string m_what;
};