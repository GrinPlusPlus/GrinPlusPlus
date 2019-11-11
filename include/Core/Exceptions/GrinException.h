#pragma once

#include <stdexcept>
#include <string>

class GrinException : public std::exception
{
public:
	virtual ~GrinException() = default;

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

	const std::string& GetMsg() const { return m_message; }

protected:
	GrinException(const std::string& message, const std::string& function)
	{
		m_message = message;
		m_function = function;
		m_what = function + ": " + message;
	}

private:
	std::string m_message;
	std::string m_function;
	std::string m_what;
};