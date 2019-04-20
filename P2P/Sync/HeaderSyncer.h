#pragma once

#include <chrono>

// Forward Declarations
class ConnectionManager;
class IBlockChainServer;
class SyncStatus;

class HeaderSyncer
{
public:
	HeaderSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	bool SyncHeaders(const SyncStatus& syncStatus, const bool startup);

private:
	bool IsHeaderSyncDue(const SyncStatus& syncStatus);
	bool RequestHeaders(const SyncStatus& syncStatus);

	ConnectionManager & m_connectionManager;
	IBlockChainServer& m_blockChainServer;

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_lastHeight;
	uint64_t m_connectionId;
	bool m_retried;
};