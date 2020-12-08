#pragma once

#include <Core/Exceptions/GrinException.h>

#define TXHASHSET_EXCEPTION(msg) TxHashSetException(msg, __func__)
#define TXHASHSET_EXCEPTION_F(msg, ...) TxHashSetException(StringUtil::Format(msg, __VA_ARGS__), __func__)

class TxHashSetException : public GrinException
{
public:
	TxHashSetException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};