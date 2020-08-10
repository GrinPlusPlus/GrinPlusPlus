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

	if (networkHeight >= (chainHeight + 5) || (startup && networkHeight > chainHeight))
	{
		if (IsHeaderSyncDue(syncStatus))
		{
			RequestHeaders(syncStatus);
		}

		return true;
	}

	m_pPeer = nullptr;
	m_retried = false;

	return false;
}

bool HeaderSyncer::IsHeaderSyncDue(const SyncStatus& syncStatus)
{
	if (m_pPeer == nullptr)
	{
		return true;
	}

	const uint64_t height = syncStatus.GetHeaderHeight();

	// Check if headers were received, and we're ready to request next batch.
	if (height >= (m_lastHeight + P2P::MAX_BLOCK_HEADERS - 1))
	{
		LOG_TRACE("Headers received. Requesting next batch.");
		m_retried = false;
		return true;
	}

	if (!m_pConnectionManager.lock()->IsConnected(m_pPeer->GetIPAddress()))
	{
		LOG_TRACE("Peer disconnected. Requesting from new peer.");
		m_pPeer = nullptr;
		return true;
	}

	// Check if header download timed out.
	if (m_timeout < std::chrono::system_clock::now())
	{
		LOG_DEBUG("Timed out. Banning then requesting from new peer.");

		if (m_pPeer != nullptr)
		{
			if (m_retried)
			{
				LOG_ERROR_F("Banning peer {} for fraud height.", m_pPeer);
				m_pPeer->Ban(EBanReason::FraudHeight);
				m_pPeer = nullptr;
				m_retried = false;
			}
			else
			{
				m_retried = true;
			}
		}

		return true;
	}

	return false;
}

bool HeaderSyncer::RequestHeaders(const SyncStatus& syncStatus)
{
	LOG_TRACE("Requesting headers.");

	std::vector<Hash> locators = BlockLocator(m_pBlockChain).GetLocators(syncStatus);

	const GetHeadersMessage getHeadersMessage(std::move(locators));

	bool messageSent = false;
	if (m_pPeer != nullptr)
	{
		messageSent = m_pConnectionManager.lock()->SendMessageToPeer(getHeadersMessage, m_pPeer);
	}
	
	if (!messageSent)
	{
		m_pPeer = m_pConnectionManager.lock()->SendMessageToMostWorkPeer(getHeadersMessage);
	}

	if (m_pPeer != nullptr)
	{
		LOG_TRACE("Headers requested.");
		m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(12);
		m_lastHeight = syncStatus.GetHeaderHeight();
	}

	return m_pPeer != nullptr;
}