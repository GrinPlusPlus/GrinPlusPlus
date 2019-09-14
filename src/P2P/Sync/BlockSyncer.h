#pragma once

#include <chrono>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <stdint.h>

// Forward Declarations
class ConnectionManager;
class IBlockChainServer;
class SyncStatus;

class BlockSyncer
{
public:
	BlockSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	bool SyncBlocks(const SyncStatus& syncStatus, const bool startup);

private:
	bool IsBlockSyncDue(const SyncStatus& syncStatus);
	bool RequestBlocks();

	ConnectionManager & m_connectionManager;
	IBlockChainServer& m_blockChainServer;

	std::chrono::time_point<std::chrono::system_clock> m_timeout;
	uint64_t m_lastHeight;
	uint64_t m_highestRequested;

	struct RequestedBlock
	{
		uint64_t PEER_ID;
		uint64_t BLOCK_HEIGHT;
		std::chrono::time_point<std::chrono::system_clock> TIMEOUT;
	};
	std::unordered_map<uint64_t, RequestedBlock> m_requestedBlocks;
	std::unordered_set<uint64_t> m_slowPeers;
};