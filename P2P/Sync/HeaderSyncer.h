#pragma once

#include <chrono>

// Forward Declarations
class ConnectionManager;
class IBlockChainServer;

class HeaderSyncer
{
public:
	HeaderSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	bool SyncHeaders();

private:
	bool IsHeaderSyncDue() const;
	bool RequestHeaders();

	ConnectionManager & m_connectionManager;
	IBlockChainServer& m_blockChainServer;

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_lastHeight;
	uint64_t m_connectionId;
};