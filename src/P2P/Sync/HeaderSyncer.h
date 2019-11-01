#pragma once

#include <BlockChain/BlockChainServer.h>
#include <chrono>

// Forward Declarations
class ConnectionManager;
class SyncStatus;

class HeaderSyncer
{
public:
	HeaderSyncer(ConnectionManager& connectionManager, IBlockChainServerPtr pBlockChainServer);

	bool SyncHeaders(const SyncStatus& syncStatus, const bool startup);

private:
	bool IsHeaderSyncDue(const SyncStatus& syncStatus);
	bool RequestHeaders(const SyncStatus& syncStatus);

	ConnectionManager & m_connectionManager;
	IBlockChainServerPtr m_pBlockChainServer;

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_lastHeight;
	uint64_t m_connectionId;
	bool m_retried;
};