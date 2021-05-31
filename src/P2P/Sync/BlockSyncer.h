#pragma once

#include "../ConnectionManager.h"
#include "../Pipeline/Pipeline.h"

#include <BlockChain/BlockChain.h>
#include <chrono>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>

// Forward Declarations
class SyncStatus;

class BlockSyncer
{
public:
	BlockSyncer(
		const std::weak_ptr<ConnectionManager>& pConnectionManager,
		const IBlockChain::Ptr& pBlockChain,
		const std::shared_ptr<Pipeline>& pPipeline
	) : m_pConnectionManager(pConnectionManager),
		m_pBlockChain(pBlockChain),
		m_pPipeline(pPipeline),
		m_lastHeight(0),
		m_highestRequested(0) { }

	bool SyncBlocks(const SyncStatus& syncStatus, const bool startup);

private:
	bool IsBlockSyncDue(const SyncStatus& syncStatus);
	bool RequestBlocks();
	bool IsSlowPeer(PeerConstPtr pPeer) const { return m_slowPeers.find(pPeer->GetIPAddress()) != m_slowPeers.end(); }

	std::weak_ptr<ConnectionManager> m_pConnectionManager;
	IBlockChain::Ptr m_pBlockChain;
	std::shared_ptr<Pipeline> m_pPipeline;

	uint64_t m_lastHeight;
	uint64_t m_highestRequested;

	struct RequestedBlock
	{
		PeerPtr PEER;
		uint64_t BLOCK_HEIGHT;
		std::chrono::time_point<std::chrono::system_clock> TIMEOUT;
		bool RETRIED;
		// TODO: Add retry?
	};
	std::unordered_map<uint64_t, RequestedBlock> m_requestedBlocks;
	std::unordered_set<IPAddress> m_slowPeers;
};