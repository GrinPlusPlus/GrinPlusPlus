#pragma once

#include "BlockIndex.h"

#include <Core/Traits/Lockable.h>
#include <Core/DataFile.h>

// Forward Declarations
class BlockIndexAllocator;
class ChainStore;

class Chain : Traits::Batchable
{
public:
	static std::shared_ptr<Chain> Load(
		std::shared_ptr<BlockIndexAllocator> pBlockIndexAllocator,
		const EChainType chainType,
		const std::string& path,
		std::shared_ptr<const BlockIndex> pGenesisIndex
	);

	const Hash& GetHash(const uint64_t height) const;
	std::shared_ptr<const BlockIndex> GetByHeight(const uint64_t height) const;
	std::shared_ptr<const BlockIndex> GetTip() const;

	inline EChainType GetType() const { return m_chainType; }

	std::shared_ptr<const BlockIndex> AddBlock(const Hash& hash);
	void Rewind(const uint64_t lastHeight);

	virtual void Commit() override final;
	virtual void Rollback() override final;
	virtual void OnInitWrite() override final;
	virtual void OnEndWrite() override final;

private:
	Chain(
		const EChainType chainType,
		std::shared_ptr<BlockIndexAllocator> pBlockIndexAllocator,
		std::shared_ptr<DataFile<32>> pDataFile,
		std::vector<std::shared_ptr<const BlockIndex>>&& indices
	);

	const EChainType m_chainType;
	std::shared_ptr<BlockIndexAllocator> m_pBlockIndexAllocator;
	std::vector<std::shared_ptr<const BlockIndex>> m_indices;
	size_t m_height;
	Locked<DataFile<32>> m_dataFile;
	Writer<DataFile<32>> m_dataFileWriter;
};

class BlockIndexAllocator
{
public:
	BlockIndexAllocator() = default;

	void AddChain(std::shared_ptr<const Chain> pChain)
	{
		m_chains.push_back(pChain);
	}

	std::shared_ptr<const BlockIndex> GetOrCreateIndex(const Hash& hash, const uint64_t height) const
	{
		return GetOrCreateIndex(Hash(hash), height);
	}

	std::shared_ptr<const BlockIndex> GetOrCreateIndex(Hash&& hash, const uint64_t height) const
	{
		for (std::shared_ptr<const Chain> pChain : m_chains)
		{
			std::shared_ptr<const BlockIndex> pIndex = pChain->GetByHeight(height);
			if (pIndex != nullptr && pIndex->GetHash() == hash)
			{
				return pIndex;
			}
		}

		return std::make_shared<const BlockIndex>(BlockIndex(std::move(hash), height));
	}

private:
	std::vector<std::shared_ptr<const Chain>> m_chains;
};