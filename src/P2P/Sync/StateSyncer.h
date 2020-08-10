#pragma once

#include "../ConnectionManager.h"

#include <BlockChain/BlockChain.h>
#include <chrono>

// Forward Declarations
class SyncStatus;

class StateSyncer
{
public:
	StateSyncer(const std::weak_ptr<ConnectionManager>& pConnectionManager, const IBlockChain::Ptr& pBlockChain)
		: m_pConnectionManager(pConnectionManager), m_pBlockChain(pBlockChain)
	{
		m_timeRequested = std::chrono::system_clock::now();
		m_requestedHeight = 0;
		m_pPeer = nullptr;
	}

	bool SyncState(SyncStatus& syncStatus);

private:
	bool IsStateSyncDue(const SyncStatus& syncStatus) const;
	bool RequestState(const SyncStatus& syncStatus);

	std::chrono::time_point<std::chrono::system_clock> m_timeRequested;
	uint64_t m_requestedHeight;
	PeerPtr m_pPeer;

	std::weak_ptr<ConnectionManager> m_pConnectionManager;
	IBlockChain::Ptr m_pBlockChain;
};