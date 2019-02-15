#pragma once

#include <exception>

class RestException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Rest exception occurred.";
	}
};