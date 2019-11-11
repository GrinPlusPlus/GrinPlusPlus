#pragma once

#include <Core/Exceptions/GrinException.h>
#include <Common/Util/StringUtil.h>

#define WALLET_EXCEPTION(msg) WalletException(msg, __FUNCTION__)
#define WALLET_EXCEPTION_F(msg, ...) WalletException(StringUtil::Format(msg, __VA_ARGS__), __FUNCTION__)

class WalletException : public GrinException
{
public:
	WalletException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}

	virtual ~WalletException() = default;
};