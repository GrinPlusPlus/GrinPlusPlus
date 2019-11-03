#pragma once

#include "BlockIndex.h"

#include <Core/Traits/Lockable.h>
#include <Core/DataFile.h>

// Forward Declarations
class BlockIndexAllocator;
class ChainStore;

class Chain
{
public:
	static std::shared_ptr<Chain> Load(BlockIndexAllocator& blockIndexAllocator, const EChainType chainType, const std::string& path, BlockIndex* pGenesisIndex);

	BlockIndex* GetByHeight(const uint64_t height);
	const BlockIndex* GetByHeight(const uint64_t height) const;

	BlockIndex* GetTip();
	const BlockIndex* GetTip() const;

	inline EChainType GetType() const { return m_chainType; }

	bool AddBlock(BlockIndex* pBlockIndex);
	bool Rewind(const uint64_t lastHeight);
	bool Flush();

private:
	Chain(const EChainType chainType, std::shared_ptr<DataFile<32>> pDataFile, std::vector<BlockIndex*>&& indices);

	Writer<DataFile<32>> GetTransaction();

	const EChainType m_chainType;
	std::vector<BlockIndex*> m_indices;
	size_t m_height;
	Locked<DataFile<32>> m_pDataFile;
	Writer<DataFile<32>> m_transaction;
};

class BlockIndexAllocator
{
public:
	explicit BlockIndexAllocator(const std::vector<std::shared_ptr<Chain>>& chains)
		: m_chains(chains)
	{

	}

	void AddChain(std::shared_ptr<Chain> pChain)
	{
		m_chains.push_back(pChain);
	}

	BlockIndex* GetOrCreateIndex(const Hash& hash, const uint64_t height)
	{
		for (std::shared_ptr<Chain> pChain : m_chains)
		{
			BlockIndex* pIndex = pChain->GetByHeight(height);
			if (pIndex != nullptr && pIndex->GetHash() == hash)
			{
				return pIndex;
			}
		}

		return new BlockIndex(hash, height);
	}

private:
	std::vector<std::shared_ptr<Chain>> m_chains;
};