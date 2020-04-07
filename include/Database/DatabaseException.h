#pragma once

#include <stdexcept>
#include <Common/Util/StringUtil.h>

#define DATABASE_EXCEPTION(msg) DatabaseException(msg, __func__)
#define DATABASE_EXCEPTION_F(msg, ...) DatabaseException(StringUtil::Format(msg, __VA_ARGS__), __func__)

class DatabaseException : public std::exception
{
public:
	DatabaseException(const std::string& message, const std::string& function)
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