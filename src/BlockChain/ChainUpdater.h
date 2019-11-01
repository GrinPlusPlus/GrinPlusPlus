#pragma once

#include "BlockStore.h"
#include "ChainStore.h"
#include "OrphanPool/OrphanPool.h"

#include <PMMR/HeaderMMR.h>
#include <TxPool/TransactionPool.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <shared_mutex>
#include <map>

class ChainUpdater
{
public:
	ChainUpdater(
		std::shared_mutex& mutex,
		std::shared_ptr<ChainStore> pChainStore,
		BlockStore& blockStore,
		std::shared_ptr<Locked<IHeaderMMR>>& pHeaderMMR,
		OrphanPool& orphanPool,
		ITransactionPool& transactionPool,
		TxHashSetManager& txHashSetManager
	)
		: m_pReferences(new int(1)),
		m_mutex(mutex),
		m_pChainStore(pChainStore),
		m_blockStore(blockStore),
		m_pHeaderMMR(pHeaderMMR),
		m_orphanPool(orphanPool),
		m_transactionPool(transactionPool),
		m_txHashSetManager(txHashSetManager)
	{
		m_mutex.lock();
	}

	~ChainUpdater()
	{
		if (--(*m_pReferences) == 0)
		{
			m_mutex.unlock();
			delete m_pReferences;
		}
	}

	ChainUpdater(const ChainUpdater& other)
		: m_pReferences(other.m_pReferences),
		m_mutex(other.m_mutex),
		m_pChainStore(other.m_pChainStore),
		m_blockStore(other.m_blockStore),
		m_pHeaderMMR(other.m_pHeaderMMR),
		m_orphanPool(other.m_orphanPool),
		m_transactionPool(other.m_transactionPool),
		m_txHashSetManager(other.m_txHashSetManager)
	{
		++(*m_pReferences);
	}
	ChainUpdater& operator=(const ChainUpdater&) = delete;

	int* m_pReferences;
	std::shared_mutex& m_mutex;
	std::shared_ptr<ChainStore> m_pChainStore;
	BlockStore& m_blockStore;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
	OrphanPool& m_orphanPool;
	ITransactionPool& m_transactionPool;
	TxHashSetManager& m_txHashSetManager;
}; #pragma once

#include "BlockStore.h"
#include "ChainStore.h"
#include "OrphanPool/OrphanPool.h"

#include <PMMR/HeaderMMR.h>
#include <TxPool/TransactionPool.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <shared_mutex>
#include <map>

class ChainUpdater
{
public:
	ChainUpdater(
		std::shared_mutex& mutex,
		std::shared_ptr<ChainStore> pChainStore,
		BlockStore& blockStore,
		std::shared_ptr<Locked<IHeaderMMR>>& pHeaderMMR,
		OrphanPool& orphanPool,
		ITransactionPool& transactionPool,
		TxHashSetManager& txHashSetManager
	)
		: m_pReferences(new int(1)),
		m_mutex(mutex),
		m_pChainStore(pChainStore),
		m_blockStore(blockStore),
		m_pHeaderMMR(pHeaderMMR),
		m_orphanPool(orphanPool),
		m_transactionPool(transactionPool),
		m_txHashSetManager(txHashSetManager)
	{
		m_mutex.lock();
	}

	~ChainUpdater()
	{
		m_mutex.unlock();
	}

	ChainUpdater(const ChainUpdater& other) = delete;
	ChainUpdater& operator=(const ChainUpdater&) = delete;

	int* m_pReferences;
	std::shared_mutex& m_mutex;
	std::shared_ptr<ChainStore> m_pChainStore;
	BlockStore& m_blockStore;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
	OrphanPool& m_orphanPool;
	ITransactionPool& m_transactionPool;
	TxHashSetManager& m_txHashSetManager;
};