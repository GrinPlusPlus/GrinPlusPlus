#include "HeaderSyncer.h"
#include "../BlockLocator.h"
#include "../Messages/GetHeadersMessage.h"

#include <Common/Logger.h>

bool HeaderSyncer::SyncHeaders(const SyncStatus& syncStatus, const bool startup)
{
	const uint64_t chainHeight = syncStatus.GetHeaderHeight();
	const uint64_t networkHeight = syncStatus.GetNetworkHeight();

	// This solves an issue that occurs due to the handshake not containing height.
	if (networkHeight == 0)
	{
		return true;
	}

	if (networkHeight > chainHeight + 15  || (startup && networkHeight > chainHeight))
	{
		if (IsHeaderSyncDue(syncStatus.GetHeaderHeight()))
		{
			if (RequestHeaders(syncStatus))
			{
				LOG_TRACE_F("Headers requested successfully to: {}", *m_pPeer);
			}
		}

		return true;
	}

	m_pPeer = nullptr;

	return false;
}

bool HeaderSyncer::IsHeaderSyncDue(const uint64_t height)
{
	if (m_pPeer == nullptr)
	{
		return true;
	}

	// Check if headers were received, and we're ready to request next batch.
	if (height > m_lastHeight + P2P::MAX_BLOCK_HEADERS)
	{
		LOG_TRACE("Headers received. Requesting next batch.");
		m_retried = false;
		return true;
	}

	// Check if header download timed out.
	if (m_timeout < std::chrono::system_clock::now())
	{
		LOG_TRACE("Timed out... requesting headers from new peer.");
		m_pPeer = nullptr;
		return true;
	}

	return false;
}

bool HeaderSyncer::RequestHeaders(const SyncStatus& syncStatus)
{
	std::vector<Hash> locators = BlockLocator(m_pBlockChain).GetLocators(syncStatus);
	const GetHeadersMessage getHeadersMessage(std::move(locators));
	
	if (m_pPeer != nullptr)
	{
		if (!m_pConnectionManager.lock()->ExchangeMessageWithPeer(getHeadersMessage, m_pPeer))
		{
			m_pPeer = nullptr;
		}
	}
	
	if (m_pPeer == nullptr)
	{
		std::pair<PeerPtr, std::unique_ptr<RawMessage>> attemptResults = m_pConnectionManager.lock()->ExchangeMessageWithMostWorkPeer(getHeadersMessage);
		if (attemptResults.first == nullptr)
		{
			LOG_TRACE_F("Unable to request Headers");
			return false;
		}
		if (attemptResults.second == nullptr)
		{
			LOG_TRACE_F("{} did not answer back", *attemptResults.first);
			return false;
		}

		m_pPeer = attemptResults.first;

		if (attemptResults.second->GetMessageType() != MessageTypes::Headers)
		{
			LOG_TRACE_F("{} did not sent headers back, received: {}", *attemptResults.first, attemptResults.second->GetMessageHeader());
			return false;
		}
	}

	m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
	m_lastHeight = syncStatus.GetHeaderHeight();

	return m_pPeer != nullptr;
}
