#pragma once

#include <Core/Exceptions/GrinException.h>

#define DESERIALIZATION_EXCEPTION() DeserializationException("Attempted to read past end of ByteBuffer.", __FUNCTION__)

class DeserializationException : public GrinException
{
public:
	DeserializationException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};