#pragma once

#include <exception>

class DeserializationException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Attempted to read past end of ByteBuffer.";
	}
};