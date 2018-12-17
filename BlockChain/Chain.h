#pragma once

#include "BlockIndex.h"

class Chain
{
public:
	Chain(const EChainType chainType, BlockIndex* pGenesisBlock);

	BlockIndex* GetByHeight(const uint64_t height);
	BlockIndex* GetTip();
	inline EChainType GetType() const { return m_chainType; }

	bool AddBlock(BlockIndex* pBlockIndex);
	bool Rewind(const uint64_t lastHeight);

private:
	const EChainType m_chainType;
	std::vector<BlockIndex*> m_indices;
	size_t m_height;
};