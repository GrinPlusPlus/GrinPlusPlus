#pragma once

#include "BlockStore.h"
#include "ChainStore.h"
#include "OrphanPool.h"

#include <HeaderMMR.h>
#include <Core/BlockHeader.h>
#include <Infrastructure/Logger.h>
#include <Hash.h>
#include <shared_mutex>
#include <map>

class LockedChainState
{
public:
	LockedChainState(std::shared_mutex& mutex, ChainStore& chainStore, BlockStore& blockStore, IHeaderMMR& headerMMR, OrphanPool& orphanPool)
		: m_pReferences(new int(1)), m_mutex(mutex), m_chainStore(chainStore), m_blockStore(blockStore), m_headerMMR(headerMMR), m_orphanPool(orphanPool)
	{
		m_mutex.lock();
		LoggerAPI::LogInfo("LockedChainState - Mutex Locked.");
	}

	~LockedChainState()
	{
		if (--(*m_pReferences) == 0)
		{
			m_mutex.unlock();
			LoggerAPI::LogInfo("LockedChainState - Mutex Unlocked.");
			delete m_pReferences;
		}
	}

	LockedChainState(const LockedChainState& other)
		: m_pReferences(other.m_pReferences),
		m_mutex(other.m_mutex),
		m_chainStore(other.m_chainStore),
		m_blockStore(other.m_blockStore),
		m_headerMMR(other.m_headerMMR),
		m_orphanPool(other.m_orphanPool)
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
};