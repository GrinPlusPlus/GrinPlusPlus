#pragma once

#include <Core/Exceptions/GrinException.h>
#include <Common/Util/StringUtil.h>

#define KEYCHAIN_EXCEPTION(msg) KeyChainException(msg, __func__)
#define KEYCHAIN_EXCEPTION_F(msg, ...) KeyChainException(StringUtil::Format(msg, __VA_ARGS__), __func__)

class KeyChainException : public GrinException
{
public:
	KeyChainException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};