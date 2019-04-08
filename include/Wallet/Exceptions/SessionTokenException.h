#pragma once

#include <exception>

class SessionTokenException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Invalid Session Token.";
	}
};