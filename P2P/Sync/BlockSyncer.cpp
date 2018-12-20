#include "BlockSyncer.h"
#include "../ConnectionManager.h"
#include "../Messages/GetBlockMessage.h"

#include <BlockChainServer.h>
#include <Infrastructure/Logger.h>

BlockSyncer::BlockSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer)
	: m_connectionManager(connectionManager), m_blockChainServer(blockChainServer)
{
	m_timeout = std::chrono::system_clock::now();
	m_lastHeight = 0;
	m_connectionId = 0;
}

// TODO: This is just a quick prototype to make sure the block processing logic is right. Need to actually implement this
bool BlockSyncer::SyncBlocks()
{
	const uint64_t height = m_blockChainServer.GetHeight(EChainType::CONFIRMED);
	const uint64_t highestHeight = m_connectionManager.GetHighestHeight();

	if (height > 0 && highestHeight >= (height + 5))
	{
		if (IsBlockSyncDue())
		{
			RequestBlocks();
		}

		return true;
	}

	return false;
}

bool BlockSyncer::IsBlockSyncDue() const
{
	const uint64_t height = m_blockChainServer.GetHeight(EChainType::CANDIDATE);
	const uint64_t highestHeight = m_connectionManager.GetHighestHeight();

	if (height > 0 && highestHeight >= (height + 5))
	{
		// Check if block download timed out.
		if (m_timeout < std::chrono::system_clock::now())
		{
			LoggerAPI::LogWarning("BlockSyncer::IsBlockSyncDue() - Timed out. Requesting again.");
			// TODO: Choose new sync peer
			return true;
		}

		// Check if blocks were received, and we're ready to request next batch.
		if (height >= (m_lastHeight + 1))
		{
			LoggerAPI::LogWarning("BlockSyncer::IsBlockSyncDue() - Blocks received. Requesting next batch.");
			return true;
		}
	}

	return false;
}

bool BlockSyncer::RequestBlocks()
{
	LoggerAPI::LogWarning("BlockSyncer: Requesting blocks.");

	const uint64_t chainHeight = m_blockChainServer.GetHeight(EChainType::CONFIRMED);
	std::unique_ptr<BlockHeader> pNextHeader = m_blockChainServer.GetBlockHeaderByHeight(chainHeight + 1, EChainType::CANDIDATE);
	if (pNextHeader != nullptr)
	{
		const GetBlockMessage getBlockMessage(pNextHeader->GetHash());
		m_connectionId = m_connectionManager.SendMessageToMostWorkPeer(getBlockMessage);

		if (m_connectionId != 0)
		{
			LoggerAPI::LogWarning("BlockSyncer: Blocks requested.");
			m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
			m_lastHeight = chainHeight;
		}

		return m_connectionId != 0;
	}

	return false;
}