#pragma once

#include <Core/FullBlock.h>

class Environment
{
public:
	Environment(const FullBlock& block)
		: m_block(std::move(block))
	{

	}

	inline const FullBlock& GetGenesisBlock() const { return m_block; }
	inline const Hash& GetGenesisHash() const { return m_block.GetBlockHeader().GetHash(); }

private:
	FullBlock m_block;
};