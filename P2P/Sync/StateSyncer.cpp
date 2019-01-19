#include "StateSyncer.h"
#include "../BlockLocator.h"
#include "../ConnectionManager.h"
#include "../Messages/TxHashSetRequestMessage.h"

#include <BlockChainServer.h>
#include <Consensus/BlockTime.h>

StateSyncer::StateSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer)
	: m_connectionManager(connectionManager), m_blockChainServer(blockChainServer)
{
	m_timeout = std::chrono::system_clock::now();
	m_requestedHeight = 0;
}

bool StateSyncer::SyncState(const SyncStatus& syncStatus)
{
	if (IsStateSyncDue(syncStatus))
	{
		RequestState(syncStatus);

		return true;
	}

	// If state sync is still in progress, return true to delay block sync.
	if (syncStatus.GetBlockHeight() < m_requestedHeight)
	{
		return true;
	}

	return false;
}

// NOTE: This doesn't handle re-orgs beyond the horizon.
bool StateSyncer::IsStateSyncDue(const SyncStatus& syncStatus) const
{
	const uint64_t headerHeight = syncStatus.GetHeaderHeight();
	const uint64_t blockHeight = syncStatus.GetBlockHeight();

	// For the first week, there's no reason to request TxHashSet, since we can just download full blocks.
	if (headerHeight < Consensus::CUT_THROUGH_HORIZON)
	{
		return false;
	}

	// If block height is within threshold, just rely on block sync.
	if (blockHeight > (headerHeight - Consensus::STATE_SYNC_THRESHOLD))
	{
		return false;
	}

	if (m_requestedHeight > 0)
	{
		// If state sync has already occurred, just rely on block sync.
		if (blockHeight >= m_requestedHeight)
		{
			return false;
		}

		// If TxHashSet download hasn't timed out, wait for it to finish.
		if (m_timeout > std::chrono::system_clock::now())
		{
			return false;
		}
	}

	return true;
}

bool StateSyncer::RequestState(const SyncStatus& syncStatus)
{
	const uint64_t headerHeight = syncStatus.GetHeaderHeight();
	const uint64_t requestedHeight = headerHeight - Consensus::STATE_SYNC_THRESHOLD;
	Hash hash = m_blockChainServer.GetBlockHeaderByHeight(requestedHeight, EChainType::CANDIDATE)->GetHash();

	const TxHashSetRequestMessage txHashSetRequestMessage(std::move(hash), requestedHeight);
	const bool requested = m_connectionManager.SendMessageToMostWorkPeer(txHashSetRequestMessage);

	if (requested)
	{
		m_timeout = std::chrono::system_clock::now() + std::chrono::minutes(10);
		m_requestedHeight = requestedHeight;
	}

	return requested;
}