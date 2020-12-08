#pragma once

#include <BlockChain/BlockIndex.h>
#include <Core/Models/BlockHeader.h>
#include <BlockChain/ChainType.h>
#include <Core/Traits/Lockable.h>
#include <Core/File/DataFile.h>

// Forward Declarations
class BlockIndexAllocator;
class ChainStore;

class Chain : public Traits::IBatchable
{
public:
	using Ptr = std::shared_ptr<Chain>;
	using CPtr = std::shared_ptr<const Chain>;

	static Chain::Ptr Load(
		std::shared_ptr<BlockIndexAllocator> pBlockIndexAllocator,
		const EChainType chainType,
		const fs::path& path,
		std::shared_ptr<const BlockIndex> pGenesisIndex
	);

	std::shared_ptr<const BlockIndex> GetByHeight(const uint64_t height) const;

	const Hash& GetHash(const uint64_t height) const { return m_indices[height]->GetHash(); }
	std::shared_ptr<const BlockIndex> GetTip() const { return m_indices[m_height]; }
	const Hash& GetTipHash() const { return m_indices[m_height]->GetHash(); }
	uint64_t GetHeight() const { return m_height; }

	bool IsOnChain(const uint64_t height, const Hash& hash) const noexcept
	{
		return height <= m_height && m_indices[height]->GetHash() == hash;
	}

	bool IsOnChain(const BlockHeaderPtr& pHeader) const noexcept
	{
		return IsOnChain(pHeader->GetHeight(), pHeader->GetHash());
	}

	EChainType GetType() const noexcept { return m_chainType; }

	std::shared_ptr<const BlockIndex> AddBlock(const Hash& hash, const uint64_t height);
	void Rewind(const uint64_t lastHeight);

	void Commit() final;
	void Rollback() noexcept final;
	void OnInitWrite(const bool batch) final;
	void OnEndWrite() final;

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

	void AddChain(const std::weak_ptr<const Chain>& pChain)
	{
		m_chains.push_back(pChain);
	}

	std::shared_ptr<const BlockIndex> GetOrCreateIndex(const Hash& hash, const uint64_t height) const
	{
		return GetOrCreateIndex(Hash(hash), height);
	}

	std::shared_ptr<const BlockIndex> GetOrCreateIndex(Hash&& hash, const uint64_t height) const
	{
		for (std::weak_ptr<const Chain> pChain : m_chains)
		{
			std::shared_ptr<const BlockIndex> pIndex = pChain.lock()->GetByHeight(height);
			if (pIndex != nullptr && pIndex->GetHash() == hash)
			{
				return pIndex;
			}
		}

		return std::make_shared<const BlockIndex>(BlockIndex(std::move(hash), height));
	}

private:
	std::vector<std::weak_ptr<const Chain>> m_chains;
};