#include "StateSyncer.h"
#include "../Messages/TxHashSetRequestMessage.h"

#include <Consensus.h>
#include <Common/Logger.h>
#include <Core/Global.h>

bool StateSyncer::SyncState(SyncStatus& syncStatus)
{
	if (IsStateSyncDue(syncStatus) && (Global::IsRunning()))
	{
		syncStatus.UpdateStatus(ESyncStatus::SYNCING_TXHASHSET);
		if (m_requestedHeight == 0)
		{
			RequestState(syncStatus);
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
	const uint64_t headerHeight = syncStatus.GetHeaderHeight();
	const uint64_t blockHeight = syncStatus.GetBlockHeight();

	const ESyncStatus status = syncStatus.GetStatus();
	if (status == ESyncStatus::PROCESSING_TXHASHSET)
	{
		return false;
	}

	if (status == ESyncStatus::TXHASHSET_SYNC_FAILED)
	{
		LOG_WARNING("TxHashSet sync failed.");
		m_requestedHeight = 0;
		return true;
	}

	// For the first week, there's no reason to request TxHashSet, since we can just download full blocks.
	if (headerHeight < Consensus::CUT_THROUGH_HORIZON)
	{
		return false;
	}

	// If block height is within threshold, just rely on block sync.
	if (blockHeight > (headerHeight - Consensus::CUT_THROUGH_HORIZON))
	{
		return false;
	}

	// If TxHashSet download timed out, request it from another peer.
	if ((m_timeRequested + std::chrono::minutes(120)) < std::chrono::system_clock::now())
	{
		LOG_WARNING("Download timed out (120 minutes).");
		m_requestedHeight = 0;
		return true;
	}

	// If 10 seconds elapsed with no progress, try another peer.
	if ((m_timeRequested + std::chrono::seconds(30)) < std::chrono::system_clock::now())
	{
		const uint64_t downloaded = syncStatus.GetDownloaded();
		if (downloaded == 0)
		{
			LOG_WARNING("30 seconds elapsed and download still not started.");
			m_requestedHeight = 0;
			return true;
		}
	}

	if (m_pPeer != nullptr && !m_pConnectionManager.lock()->IsConnected(m_pPeer->GetIPAddress()))
	{
		LOG_WARNING("Sync peer no longer connected.");
		m_requestedHeight = 0;
		return true;
	}

	return false;
}

bool StateSyncer::RequestState(const SyncStatus& syncStatus)
{
	LOG_DEBUG("Requesting Hashset...");
	/*if (m_pPeer != nullptr)
	{
		LOG_WARNING_F("Banning peer: {}", m_pPeer);
		m_pPeer->Ban(EBanReason::BadTxHashSet);
		m_pPeer = nullptr;
	}*/
	
	const uint64_t headerHeight = syncStatus.GetHeaderHeight();
	const uint64_t requestedHeight = headerHeight - Consensus::STATE_SYNC_THRESHOLD;
	
	Hash hash = m_pBlockChain->GetBlockHeaderByHeight(requestedHeight, EChainType::CANDIDATE)->GetHash();

	const TxHashSetRequestMessage txHashSetRequestMessage(std::move(hash), requestedHeight);
	
	std::pair<PeerPtr, std::unique_ptr<RawMessage>> result = m_pConnectionManager.lock()->ExchangeMessageWithMostWorkPeer(txHashSetRequestMessage);

	if (result.first == nullptr)
	{
		LOG_WARNING_F("Unable to request Headers");
		return false;
	}

	if (result.second == nullptr)
	{
		LOG_WARNING_F("{} did not answer back", *result.first);
		return false;
	}

	if (result.second->GetMessageType() != MessageTypes::TxHashSetArchive)
	{
		LOG_WARNING_F("{} did not sent hashset back, received: {}", *result.first, result.second->GetMessageHeader());
		return false;
	}

	m_pPeer = result.first;
	m_timeRequested = std::chrono::system_clock::now();
	m_requestedHeight = requestedHeight;

	LOG_DEBUG_F("TxHashSetRequestMessage sent to: {}", m_pPeer->GetIPAddress());
	
	return true;
}