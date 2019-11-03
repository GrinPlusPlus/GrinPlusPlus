#pragma once

#include <Core/Exceptions/GrinException.h>

#define TXHASHSET_EXCEPTION(msg) TxHashSetException(msg, __FUNCTION__)

class TxHashSetException : public GrinException
{
public:
	TxHashSetException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};