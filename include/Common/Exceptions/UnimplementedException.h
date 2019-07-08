#pragma once

#include <stdexcept>
#include <string>

#define UNIMPLEMENTED_EXCEPTION UnimplementedException("Not Implemented", __FUNCTION__)

class UnimplementedException : public std::logic_error
{
public:
	UnimplementedException(const std::string& message, const std::string& function)
		: std::logic_error("Not Implemented")
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