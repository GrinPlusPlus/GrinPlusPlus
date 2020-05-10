#pragma once

#include <Core/Exceptions/GrinException.h>
#include <Common/Util/StringUtil.h>

#define DESERIALIZATION_EXCEPTION(msg) DeserializationException(msg, __func__)
#define DESERIALIZATION_EXCEPTION_F(msg, ...) DeserializationException(StringUtil::Format(msg, __VA_ARGS__), __func__)
#define DESERIALIZE_FIELD_EXCEPTION(field) DeserializationException(StringUtil::Format("Failed to deserialize {}", field), __func__)

class DeserializationException : public GrinException
{
public:
	DeserializationException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};