#pragma once

#include <Core/Exceptions/GrinException.h>
#include <Common/Util/StringUtil.h>

#define FILE_EXCEPTION(msg) FileException(msg, __FUNCTION__)
#define FILE_EXCEPTION_F(msg, ...) FileException(StringUtil::Format(msg, __VA_ARGS__), __FUNCTION__)

class FileException : public GrinException
{
public:
	FileException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};