#pragma once

#include <chrono>

// Forward Declarations
class ConnectionManager;
class IBlockChainServer;
class SyncStatus;

class StateSyncer
{
public:
	StateSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	bool SyncState(const SyncStatus& syncStatus);

private:
	bool IsStateSyncDue(const SyncStatus& syncStatus) const;
	bool RequestState(const SyncStatus& syncStatus);

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_requestedHeight;

	ConnectionManager & m_connectionManager;
	IBlockChainServer& m_blockChainServer;
};