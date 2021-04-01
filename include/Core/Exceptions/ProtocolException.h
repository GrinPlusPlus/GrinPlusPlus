#pragma once

#include <Core/Exceptions/GrinException.h>
#include <Common/Util/StringUtil.h>

#define PROTOCOL_EXCEPTION(msg) ProtocolException(msg, __func__)
#define PROTOCOL_EXCEPTION_F(msg, ...) ProtocolException(StringUtil::Format(msg, __VA_ARGS__), __func__)

class ProtocolException : public GrinException
{
public:
	ProtocolException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};