#pragma once

#include <Common/Util/StringUtil.h>
#include <Core/Exceptions/GrinException.h>

#define BLOCK_CHAIN_EXCEPTION(msg) BlockChainException(msg, __func__)
#define BLOCK_CHAIN_EXCEPTION_F(msg, ...) BlockChainException(StringUtil::Format(msg, __VA_ARGS__), __func__)

class BlockChainException : public GrinException
{
public:
	BlockChainException(const std::string& message, const std::string& function)
		: GrinException(message, function)
	{

	}
};