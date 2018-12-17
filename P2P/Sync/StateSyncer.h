#pragma once

#include <chrono>

// Forward Declarations
class ConnectionManager;
class IBlockChainServer;

class StateSyncer
{
public:
	StateSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	bool SyncState();

private:
	bool IsStateSyncDue() const;
	bool RequestState();

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_requestedHeight;

	ConnectionManager & m_connectionManager;
	IBlockChainServer& m_blockChainServer;
};