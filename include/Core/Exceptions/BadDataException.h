#pragma once

#include <Core/Exceptions/GrinException.h>

#define BAD_DATA_EXCEPTION(msg) BadDataException(msg, __FUNCTION__)

class BadDataException : public GrinException
{
public:
	BadDataException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};