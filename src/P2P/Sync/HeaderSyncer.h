#pragma once

#include "../ConnectionManager.h"

#include <BlockChain/BlockChainServer.h>
#include <chrono>

// Forward Declarations
class SyncStatus;

class HeaderSyncer
{
public:
	HeaderSyncer(std::weak_ptr<ConnectionManager> pConnectionManager, IBlockChainServerPtr pBlockChainServer);

	bool SyncHeaders(const SyncStatus& syncStatus, const bool startup);

private:
	bool IsHeaderSyncDue(const SyncStatus& syncStatus);
	bool RequestHeaders(const SyncStatus& syncStatus);

	std::weak_ptr<ConnectionManager> m_pConnectionManager;
	IBlockChainServerPtr m_pBlockChainServer;

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_lastHeight;
	PeerPtr m_pPeer;
	bool m_retried;
};