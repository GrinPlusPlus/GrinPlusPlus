#include "HeaderSyncer.h"
#include "../BlockLocator.h"
#include "../ConnectionManager.h"
#include "../Messages/GetHeadersMessage.h"

#include <BlockChain/BlockChainServer.h>
#include <Infrastructure/Logger.h>

HeaderSyncer::HeaderSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer)
	: m_connectionManager(connectionManager), m_blockChainServer(blockChainServer)
{
	m_timeout = std::chrono::system_clock::now();
	m_lastHeight = 0;
	m_connectionId = 0;
	m_retried = false;
}

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

	m_connectionId = 0;
	m_retried = false;

	return false;
}

bool HeaderSyncer::IsHeaderSyncDue(const SyncStatus& syncStatus)
{
	if (m_connectionId == 0)
	{
		return true;
	}

	const uint64_t height = syncStatus.GetHeaderHeight();

	// Check if headers were received, and we're ready to request next batch.
	if (height >= (m_lastHeight + P2P::MAX_BLOCK_HEADERS - 1))
	{
		LoggerAPI::LogTrace("HeaderSyncer::IsHeaderSyncDue() - Headers received. Requesting next batch.");
		m_retried = false;
		return true;
	}

	if (!m_connectionManager.IsConnected(m_connectionId))
	{
		LoggerAPI::LogTrace("HeaderSyncer::IsHeaderSyncDue() - Peer disconnected. Requesting from new peer.");
		m_connectionId = 0;
		return true;
	}

	// Check if header download timed out.
	if (m_timeout < std::chrono::system_clock::now())
	{
		LoggerAPI::LogDebug("HeaderSyncer::IsHeaderSyncDue() - Timed out. Banning then requesting from new peer.");

		if (m_connectionId != 0)
		{
			if (m_retried)
			{
				m_connectionManager.BanConnection(m_connectionId, EBanReason::FraudHeight);
				m_connectionId = 0;
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
	LoggerAPI::LogTrace("HeaderSyncer::RequestHeaders - Requesting headers.");

	std::vector<CBigInteger<32>> locators = BlockLocator(m_blockChainServer).GetLocators(syncStatus);

	const GetHeadersMessage getHeadersMessage(std::move(locators));

	bool messageSent = false;
	if (m_connectionId != 0)
	{
		messageSent = m_connectionManager.SendMessageToPeer(getHeadersMessage, m_connectionId);
	}
	
	if (!messageSent)
	{
		m_connectionId = m_connectionManager.SendMessageToMostWorkPeer(getHeadersMessage);
	}

	if (m_connectionId != 0)
	{
		LoggerAPI::LogTrace("HeaderSyncer::RequestHeaders - Headers requested.");
		m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(12);
		m_lastHeight = syncStatus.GetHeaderHeight();
	}

	return m_connectionId != 0;
}