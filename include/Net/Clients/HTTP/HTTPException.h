#pragma once

#include <exception>
#include <string>

#define HTTP_EXCEPTION(msg) HTTPException(__func__, msg)

class HTTPException : public std::exception
{
public:
	HTTPException(const std::string& function, const std::string& message)
		: std::exception()
	{
		m_message = message;
		m_what = function + " - " + message;
	}

	HTTPException() : std::exception()
	{
		m_message = "HTTP exception occurred.";
	}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

	inline const std::string& GetMsg() const { return m_message; }

private:
	std::string m_message;
	std::string m_what;
};