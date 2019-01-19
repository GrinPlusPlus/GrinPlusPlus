#include "HeaderSyncer.h"
#include "../BlockLocator.h"
#include "../ConnectionManager.h"
#include "../Messages/GetHeadersMessage.h"

#include <BlockChainServer.h>
#include <Infrastructure/Logger.h>

HeaderSyncer::HeaderSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer)
	: m_connectionManager(connectionManager), m_blockChainServer(blockChainServer)
{
	m_timeout = std::chrono::system_clock::now();
	m_lastHeight = 0;
	m_connectionId = 0;
	m_consecutiveTimeouts = 0;
}

// TODO: Choose 1 peer to sync all headers from
bool HeaderSyncer::SyncHeaders(const SyncStatus& syncStatus)
{
	const uint64_t height = syncStatus.GetHeaderHeight();
	const uint64_t highestHeight = m_connectionManager.GetHighestHeight();

	// This solves an issue that occurs due to the handshake not containing height.
	if (highestHeight == 0)
	{
		return true;
	}

	if (highestHeight >= (height + 5))
	{
		if (IsHeaderSyncDue(syncStatus))
		{
			RequestHeaders(syncStatus);
		}

		return true;
	}

	LoggerAPI::LogError(std::to_string(height) + "  " + std::to_string(highestHeight));

	m_consecutiveTimeouts = 0;
	return false;
}

bool HeaderSyncer::IsHeaderSyncDue(const SyncStatus& syncStatus)
{
	const uint64_t height = syncStatus.GetHeaderHeight();

	// Check if header download timed out.
	if (m_timeout < std::chrono::system_clock::now())
	{
		LoggerAPI::LogWarning("HeaderSyncer::IsHeaderSyncDue() - Timed out. Requesting again.");
		++m_consecutiveTimeouts;
		// TODO: Choose new sync peer
		return true;
	}

	// Check if headers were received, and we're ready to request next batch.
	if (height >= (m_lastHeight + P2P::MAX_BLOCK_HEADERS - 1))
	{
		LoggerAPI::LogInfo("HeaderSyncer::IsHeaderSyncDue() - Headers received. Requesting next batch.");
		m_consecutiveTimeouts = 0;
		return true;
	}

	return false;
}

bool HeaderSyncer::RequestHeaders(const SyncStatus& syncStatus)
{
	LoggerAPI::LogInfo("HeaderSyncer::RequestHeaders - Requesting headers.");

	if (m_consecutiveTimeouts >= 10)
	{
		LoggerAPI::LogWarning("HeaderSyncer::RequestHeaders - Timed out 10 or more consecutive times. Banning most work peers.");
		const std::vector<uint64_t> mostWorkPeers = m_connectionManager.GetMostWorkPeers();
		for (const uint64_t mostWorkPeer : mostWorkPeers)
		{
			m_connectionManager.BanConnection(mostWorkPeer);
		}

		m_consecutiveTimeouts = 0;
	}

	const BlockLocator locator(m_blockChainServer);
	std::vector<CBigInteger<32>> locators = locator.GetLocators();

	const uint64_t chainHeight = syncStatus.GetHeaderHeight();
	const GetHeadersMessage getHeadersMessage(std::move(locators));
	m_connectionId = m_connectionManager.SendMessageToMostWorkPeer(getHeadersMessage);
	
	if (m_connectionId != 0)
	{
		LoggerAPI::LogInfo("HeaderSyncer::RequestHeaders - Headers requested.");
		m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
		m_lastHeight = chainHeight;
	}

	return m_connectionId != 0;
}