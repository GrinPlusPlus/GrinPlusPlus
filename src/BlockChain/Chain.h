#pragma once

#include "BlockIndex.h"

#include <Core/DataFile.h>

// Forward Declarations
class ChainStore;

class Chain
{
public:
	Chain(const EChainType chainType, const std::string& path, BlockIndex* pGenesisBlock);
	bool Load(ChainStore& chainStore);

	BlockIndex* GetByHeight(const uint64_t height);
	const BlockIndex* GetByHeight(const uint64_t height) const;

	BlockIndex* GetTip();
	const BlockIndex* GetTip() const;

	inline EChainType GetType() const { return m_chainType; }

	bool AddBlock(BlockIndex* pBlockIndex);
	bool Rewind(const uint64_t lastHeight);
	bool Flush();

private:
	const EChainType m_chainType;
	std::vector<BlockIndex*> m_indices;
	size_t m_height;
	DataFile<32> m_dataFile;
};