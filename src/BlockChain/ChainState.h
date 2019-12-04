#pragma once

#include "Chain.h"
#include "ChainStore.h"
#include "OrphanPool/OrphanPool.h"

#include <P2P/SyncStatus.h>
#include <Core/Models/BlockHeader.h>
#include <BlockChain/ChainType.h>
#include <Core/Models/DTOs/BlockWithOutputs.h>
#include <PMMR/HeaderMMR.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <Core/Traits/Lockable.h>
#include <Database/Database.h>
#include <TxPool/TransactionPool.h>

// Forward Declarations
class ITransactionPool;

class ChainState : public Traits::IBatchable
{
public:
	static std::shared_ptr<Locked<ChainState>> Create(
		const Config& config,
		std::shared_ptr<Locked<ChainStore>> pChainStore,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR,
		std::shared_ptr<ITransactionPool> pTransactionPool,
		std::shared_ptr<TxHashSetManager> pTxHashSetManager,
		BlockHeaderPtr pGenesisHeader
	);

	void UpdateSyncStatus(SyncStatus& syncStatus) const;
	uint64_t GetHeight(const EChainType chainType) const;
	uint64_t GetTotalDifficulty(const EChainType chainType) const;

	BlockHeaderPtr GetTipBlockHeader(const EChainType chainType) const;
	BlockHeaderPtr GetBlockHeaderByHash(const Hash& hash) const;
	BlockHeaderPtr GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const;
	BlockHeaderPtr GetBlockHeaderByCommitment(const Commitment& outputCommitment) const;

	std::unique_ptr<FullBlock> GetBlockByHash(const Hash& hash) const;
	std::unique_ptr<FullBlock> GetBlockByHeight(const uint64_t height) const;
	std::shared_ptr<const FullBlock> GetOrphanBlock(const uint64_t height, const Hash& hash) const;

	std::unique_ptr<BlockWithOutputs> GetBlockWithOutputs(const uint64_t height) const;

	std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const;

	virtual void Commit() override final;
	virtual void Rollback() override final;
	virtual void OnInitWrite() override final;
	virtual void OnEndWrite() override final;

	std::shared_ptr<ChainStore> GetChainStore()
	{
		if (m_chainStoreWriter.IsNull())
		{
			m_chainStoreWriter = m_pChainStore->BatchWrite();
		}

		return m_chainStoreWriter.GetShared();
	}

	Reader<ChainStore> GetChainStore() const
	{
		if (m_chainStoreWriter.IsNull())
		{
			return m_pChainStore->Read();
		}

		return m_chainStoreWriter;
	}

	std::shared_ptr<IBlockDB> GetBlockDB()
	{
		if (m_blockDBWriter.IsNull())
		{
			m_blockDBWriter = m_pBlockDB->BatchWrite();
		}

		return m_blockDBWriter.GetShared();
	}

	Reader<IBlockDB> GetBlockDB() const
	{
		if (m_blockDBWriter.IsNull())
		{
			return m_pBlockDB->Read();
		}

		return m_blockDBWriter;
	}

	std::shared_ptr<IHeaderMMR> GetHeaderMMR()
	{
		if (m_headerMMRWriter.IsNull())
		{
			m_headerMMRWriter = m_pHeaderMMR->BatchWrite();
		}

		return m_headerMMRWriter.GetShared();
	}

	Reader<IHeaderMMR> GetHeaderMMR() const
	{
		if (m_headerMMRWriter.IsNull())
		{
			return m_pHeaderMMR->Read();
		}

		return m_headerMMRWriter;
	}

	std::shared_ptr<ITxHashSet> GetTxHashSet()
	{
		if (m_txHashSetWriter.IsNull())
		{
			auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
			if (pTxHashSet != nullptr)
			{
				m_txHashSetWriter = pTxHashSet->BatchWrite();
			}
			else
			{
				return nullptr;
			}
		}

		return m_txHashSetWriter.GetShared();
	}

	Reader<ITxHashSet> GetTxHashSet() const
	{
		if (m_txHashSetWriter.IsNull())
		{
			auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
			if (pTxHashSet != nullptr)
			{
				return pTxHashSet->Read();
			}
			else
			{
				return Reader<ITxHashSet>::Create(nullptr, nullptr, false);
			}
		}

		return m_txHashSetWriter;
	}

	std::shared_ptr<OrphanPool> GetOrphanPool() { return m_pOrphanPool; }
	ITransactionPoolPtr GetTransactionPool() { return m_pTransactionPool; }
	TxHashSetManagerPtr GetTxHashSetManager() { return m_pTxHashSetManager; }

private:
	ChainState(
		const Config& config,
		std::shared_ptr<Locked<ChainStore>> pChainStore,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR,
		std::shared_ptr<ITransactionPool> pTransactionPool,
		std::shared_ptr<TxHashSetManager> pTxHashSetManager
	);

	const Config& m_config;
	std::shared_ptr<Locked<ChainStore>> m_pChainStore;
	std::shared_ptr<Locked<IBlockDB>> m_pBlockDB;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
	std::shared_ptr<ITransactionPool> m_pTransactionPool;
	std::shared_ptr<TxHashSetManager> m_pTxHashSetManager;
	std::shared_ptr<OrphanPool> m_pOrphanPool;

	// Writers
	Writer<ChainStore> m_chainStoreWriter;
	Writer<IBlockDB> m_blockDBWriter;
	Writer<IHeaderMMR> m_headerMMRWriter;
	Writer<ITxHashSet> m_txHashSetWriter;
};