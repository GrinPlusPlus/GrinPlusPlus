#pragma once

#include <Core/Exceptions/GrinException.h>

#define BLOCK_CHAIN_EXCEPTION(msg) BlockChainException(msg, __func__)

class BlockChainException : public GrinException
{
public:
	BlockChainException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};