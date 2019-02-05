#pragma once

#include <exception>

class CryptoException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "An error occurred while performing a cryptographic operation.";
	}
};