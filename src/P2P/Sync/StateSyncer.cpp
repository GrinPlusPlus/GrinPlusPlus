#include "StateSyncer.h"
#include "../Messages/TxHashSetRequestMessage.h"

#include <Consensus.h>
#include <Common/Logger.h>
#include <Core/Global.h>

bool StateSyncer::SyncState(SyncStatus& syncStatus)
{
	if (!Global::IsRunning()) return false;

	if (IsStateSyncDue(syncStatus))
	{
		if (RequestState(syncStatus))
		{
			LOG_INFO_F("Hashset requested successfully to: {}", *m_pPeer);
			syncStatus.UpdateStatus(ESyncStatus::SYNCING_TXHASHSET);
		}

		return true;
	}

	// If state sync is still in progress, return true to delay block sync.
	if (syncStatus.GetBlockHeight() < (syncStatus.GetHeaderHeight() - Consensus::CUT_THROUGH_HORIZON))
	{
		return true;
	}

	return false;
}

// NOTE: This doesn't handle re-orgs beyond the horizon.
bool StateSyncer::IsStateSyncDue(const SyncStatus& syncStatus)
{
	const ESyncStatus status = syncStatus.GetStatus();
	const uint64_t headerHeight = syncStatus.GetHeaderHeight();

	// If Hashset is being processed return false
	// For the first week, there's no reason to request TxHashSet, since we can just download full blocks.
	// If block height is within threshold, just rely on block sync.
	if (status == ESyncStatus::PROCESSING_TXHASHSET || 
		headerHeight < Consensus::CUT_THROUGH_HORIZON ||
		syncStatus.GetBlockHeight() >(headerHeight - Consensus::CUT_THROUGH_HORIZON)
	   )
	{
		return false;
	}

	if (m_pPeer == nullptr ||
		m_requestedHeight == 0 ||
		status == ESyncStatus::TXHASHSET_SYNC_FAILED
		)
	{
		if (status == ESyncStatus::TXHASHSET_SYNC_FAILED)
		{
			LOG_WARNING("TxHashSet sync failed.");
		}
		m_requestedHeight = 0;
		m_pPeer = nullptr;
		m_lastSize = 0;

		return true;
	}

	// If 60 seconds elapsed with no progress, try another peer.
	if (syncStatus.GetDownloaded() == 0)
	{
		if ((m_timeRequested + std::chrono::seconds(60)) < std::chrono::system_clock::now())
		{
			LOG_WARNING("60 seconds elapsed and download still not started.");
			m_requestedHeight = 0;
			m_lastSize = 0;
			m_pPeer = nullptr;
			return true;
		}
	}

	if (syncStatus.GetDownloaded() > 0)
	{
		// If TxHashSet download timed out, request it from another peer.
		if (syncStatus.GetDownloaded() == m_lastSize &&
			(m_timeRequested + std::chrono::seconds(20)) < std::chrono::system_clock::now())
		{
			m_requestedHeight = 0;
			m_lastSize = 0;
			m_pPeer = nullptr;
			return true;
		}
		
		if ((m_timeRequested + std::chrono::minutes(120)) < std::chrono::system_clock::now())
		{
			LOG_WARNING("Download timed out (120 minutes). Requesting from another peer...");
			m_requestedHeight = 0;
			m_pPeer = nullptr;
			return true;
		}
	}
	
	m_lastSize = syncStatus.GetDownloaded();

	return false;
}

bool StateSyncer::RequestState(const SyncStatus& syncStatus)
{
	const uint64_t headerHeight = syncStatus.GetHeaderHeight();
	const uint64_t requestedHeight = headerHeight - Consensus::STATE_SYNC_THRESHOLD;
	Hash hash = m_pBlockChain->GetBlockHeaderByHeight(requestedHeight, EChainType::CANDIDATE)->GetHash();
	const TxHashSetRequestMessage txHashSetRequestMessage(std::move(hash), requestedHeight);
	
	std::pair<PeerPtr, std::unique_ptr<RawMessage>> attemptResults = 
		m_pConnectionManager.lock()->ExchangeMessageWithMostWorkPeer(txHashSetRequestMessage);

	if (attemptResults.first == nullptr)
	{
		LOG_WARNING_F("Unable to request State");
		return false;
	}

	if (attemptResults.second == nullptr)
	{
		LOG_WARNING_F("{} did not answer back", *attemptResults.first);
		return false;
	}

	if (attemptResults.second->GetMessageType() != MessageTypes::TxHashSetArchive)
	{
		LOG_TRACE_F("{} did not sent hashset back, received: {}",
					  *attemptResults.first, attemptResults.second->GetMessageHeader());
		return false;
	}

	m_pPeer = attemptResults.first;
	m_timeRequested = std::chrono::system_clock::now();
	m_requestedHeight = requestedHeight;
	m_lastSize = 0;

	LOG_DEBUG_F("TxHashSetRequestMessage sent to: {}", m_pPeer->GetIPAddress());
	
	return true;
}