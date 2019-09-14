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

	std::chrono::time_point<std::chrono::system_clock> m_timeRequested;
	uint64_t m_requestedHeight;
	uint64_t m_connectionId;

	ConnectionManager & m_connectionManager;
	IBlockChainServer& m_blockChainServer;
};