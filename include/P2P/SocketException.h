#pragma once

#include <exception>

class SocketException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Socket exception occurred.";
	}
};