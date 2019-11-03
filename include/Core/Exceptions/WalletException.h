#pragma once

#include <Core/Exceptions/GrinException.h>

#define WALLET_EXCEPTION(msg) WalletException(msg, __FUNCTION__)

class WalletException : public GrinException
{
public:
	WalletException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};