#pragma once

#include "../ConnectionManager.h"

#include <BlockChain/BlockChainServer.h>
#include <chrono>

// Forward Declarations
class SyncStatus;

class StateSyncer
{
public:
	StateSyncer(std::weak_ptr<ConnectionManager> pConnectionManager, IBlockChainServerPtr pBlockChainServer);

	bool SyncState(SyncStatus& syncStatus);

private:
	bool IsStateSyncDue(const SyncStatus& syncStatus) const;
	bool RequestState(const SyncStatus& syncStatus);

	std::chrono::time_point<std::chrono::system_clock> m_timeRequested;
	uint64_t m_requestedHeight;
	PeerPtr m_pPeer;

	std::weak_ptr<ConnectionManager> m_pConnectionManager;
	IBlockChainServerPtr m_pBlockChainServer;
};