#pragma once

#include "BlockStore.h"
#include "ChainStore.h"
#include "OrphanPool/OrphanPool.h"

#include <PMMR/HeaderMMR.h>
#include <Core/Models/BlockHeader.h>
#include <Infrastructure/Logger.h>
#include <TxPool/TransactionPool.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <shared_mutex>
#include <map>

class LockedChainState
{
public:
	LockedChainState(std::shared_mutex& mutex, ChainStore& chainStore, BlockStore& blockStore, IHeaderMMR& headerMMR, OrphanPool& orphanPool, ITransactionPool& transactionPool, TxHashSetManager& txHashSetManager)
		: m_pReferences(new int(1)), 
		m_mutex(mutex), 
		m_chainStore(chainStore), 
		m_blockStore(blockStore), 
		m_headerMMR(headerMMR), 
		m_orphanPool(orphanPool),
		m_transactionPool(transactionPool),
		m_txHashSetManager(txHashSetManager)
	{
		m_mutex.lock();
		LoggerAPI::LogTrace("LockedChainState - Mutex Locked.");
	}

	~LockedChainState()
	{
		if (--(*m_pReferences) == 0)
		{
			m_mutex.unlock();
			LoggerAPI::LogTrace("LockedChainState - Mutex Unlocked.");
			delete m_pReferences;
		}
	}

	LockedChainState(const LockedChainState& other)
		: m_pReferences(other.m_pReferences),
		m_mutex(other.m_mutex),
		m_chainStore(other.m_chainStore),
		m_blockStore(other.m_blockStore),
		m_headerMMR(other.m_headerMMR),
		m_orphanPool(other.m_orphanPool),
		m_transactionPool(other.m_transactionPool),
		m_txHashSetManager(other.m_txHashSetManager)
	{
		++(*m_pReferences);
	}
	LockedChainState& operator=(const LockedChainState&) = delete;

	int* m_pReferences;
	std::shared_mutex& m_mutex;
	ChainStore& m_chainStore;
	BlockStore& m_blockStore;
	IHeaderMMR& m_headerMMR;
	OrphanPool& m_orphanPool;
	ITransactionPool& m_transactionPool;
	TxHashSetManager& m_txHashSetManager;
};