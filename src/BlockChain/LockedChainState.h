#pragma once

#include "ChainStore.h"
#include "OrphanPool/OrphanPool.h"

#include <Database/Database.h>
#include <PMMR/HeaderMMR.h>
#include <TxPool/TransactionPool.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <map>

class LockedChainState
{
public:
	LockedChainState(
		std::shared_ptr<Locked<ChainStore>> pChainStore,
		std::shared_ptr<Locked<IBlockDB>> pBlockDB,
		std::shared_ptr<Locked<IHeaderMMR>>& pHeaderMMR,
		std::shared_ptr<OrphanPool> pOrphanPool,
		std::shared_ptr<ITransactionPool> pTransactionPool,
		std::shared_ptr<TxHashSetManager> pTxHashSetManager
	)
		: m_committed(false),
		m_pChainStore(pChainStore), 
		m_pBlockDB(pBlockDB),
		m_pHeaderMMR(pHeaderMMR),
		m_pOrphanPool(pOrphanPool),
		m_pTransactionPool(pTransactionPool),
		m_pTxHashSetManager(pTxHashSetManager)
	{

	}

	~LockedChainState()
	{
		if (!m_committed)
		{
			Rollback();
		}
	}

	LockedChainState(const LockedChainState& other) = delete;
	LockedChainState& operator=(const LockedChainState&) = delete;

	void Commit()
	{
		if (!m_chainStoreWriter.IsNull())
		{
			m_chainStoreWriter->Commit();
		}

		if (!m_blockDBWriter.IsNull())
		{
			m_blockDBWriter->Commit();
		}

		if (!m_headerMMRWriter.IsNull())
		{
			m_headerMMRWriter->Commit();
		}
	}

	void Rollback()
	{
		if (!m_chainStoreWriter.IsNull())
		{
			m_chainStoreWriter->Rollback();
		}

		if (!m_blockDBWriter.IsNull())
		{
			m_blockDBWriter->Rollback();
		}

		if (!m_headerMMRWriter.IsNull())
		{
			m_headerMMRWriter->Rollback();
		}
	}

	std::shared_ptr<ChainStore> GetChainStore()
	{
		if (m_chainStoreWriter.IsNull())
		{
			m_chainStoreWriter = m_pChainStore->BatchWrite();
		}

		return m_chainStoreWriter.GetShared();
	}

	std::shared_ptr<IBlockDB> GetBlockDB()
	{
		if (m_blockDBWriter.IsNull())
		{
			m_blockDBWriter = m_pBlockDB->BatchWrite();
		}

		return m_blockDBWriter.GetShared();
	}

	std::shared_ptr<IHeaderMMR> GetHeaderMMR()
	{
		if (m_headerMMRWriter.IsNull())
		{
			m_headerMMRWriter = m_pHeaderMMR->BatchWrite();
		}

		return m_headerMMRWriter.GetShared();
	}

	std::shared_ptr<OrphanPool> GetOrphanPool()
	{
		return m_pOrphanPool;
	}

	std::shared_ptr<ITransactionPool> GetTransactionPool()
	{
		return m_pTransactionPool;
	}

	std::shared_ptr<TxHashSetManager> GetTxHashSetManager()
	{
		return m_pTxHashSetManager;
	}

private:
	bool m_committed;

	std::shared_ptr<Locked<ChainStore>> m_pChainStore;
	std::shared_ptr<Locked<IBlockDB>> m_pBlockDB;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
	std::shared_ptr<OrphanPool> m_pOrphanPool;
	std::shared_ptr<ITransactionPool> m_pTransactionPool;
	std::shared_ptr<TxHashSetManager> m_pTxHashSetManager;

	// Writers
	Writer<ChainStore> m_chainStoreWriter;
	Writer<IBlockDB> m_blockDBWriter;
	Writer<IHeaderMMR> m_headerMMRWriter;
};