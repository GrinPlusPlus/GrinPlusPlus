#pragma once

#include <stdexcept>
#include <string>

#define FILE_EXCEPTION(msg) FileException(msg, __FUNCTION__)

class FileException : public std::exception
{
public:
	FileException(const std::string& message, const std::string& function)
	{
		m_message = message;
		m_function = function;
		m_what = function + ": " + message;
	}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

private:
	std::string m_message;
	std::string m_function;
	std::string m_what;
};