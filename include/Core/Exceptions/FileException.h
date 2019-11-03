#pragma once

#include <Core/Exceptions/GrinException.h>

#define FILE_EXCEPTION(msg) FileException(msg, __FUNCTION__)

class FileException : public GrinException
{
public:
	FileException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};