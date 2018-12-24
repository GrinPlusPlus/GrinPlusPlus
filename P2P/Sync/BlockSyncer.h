#pragma once

#include <chrono>

// Forward Declarations
class ConnectionManager;
class IBlockChainServer;

class BlockSyncer
{
public:
	BlockSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	bool SyncBlocks();

private:
	bool IsBlockSyncDue();
	bool RequestBlocks();

	ConnectionManager & m_connectionManager;
	IBlockChainServer& m_blockChainServer;

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_lastHeight;
	uint64_t m_highestRequested;
};