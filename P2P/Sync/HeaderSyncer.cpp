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

	return false;
}

bool HeaderSyncer::IsHeaderSyncDue(const SyncStatus& syncStatus)
{
	const uint64_t height = syncStatus.GetHeaderHeight();

	// Check if headers were received, and we're ready to request next batch.
	if (height >= (m_lastHeight + P2P::MAX_BLOCK_HEADERS - 1))
	{
		LoggerAPI::LogTrace("HeaderSyncer::IsHeaderSyncDue() - Headers received. Requesting next batch.");
		return true;
	}

	// Check if header download timed out.
	if (m_timeout < std::chrono::system_clock::now())
	{
		LoggerAPI::LogInfo("HeaderSyncer::IsHeaderSyncDue() - Timed out. Banning then requesting from new peer.");

		if (m_connectionId != 0)
		{
			m_connectionManager.BanConnection(m_connectionId);
		}

		m_connectionId = 0;

		return true;
	}

	return false;
}

bool HeaderSyncer::RequestHeaders(const SyncStatus& syncStatus)
{
	LoggerAPI::LogInfo("HeaderSyncer::RequestHeaders - Requesting headers.");

	std::vector<CBigInteger<32>> locators = BlockLocator(m_blockChainServer).GetLocators(syncStatus);

	const GetHeadersMessage getHeadersMessage(std::move(locators));

	if (m_connectionId == 0)
	{
		m_connectionId = m_connectionManager.SendMessageToMostWorkPeer(getHeadersMessage);
	}
	else
	{
		m_connectionManager.SendMessageToPeer(getHeadersMessage, m_connectionId);
	}

	if (m_connectionId != 0)
	{
		LoggerAPI::LogInfo("HeaderSyncer::RequestHeaders - Headers requested.");
		m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
		m_lastHeight = syncStatus.GetHeaderHeight();
	}

	return m_connectionId != 0;
}