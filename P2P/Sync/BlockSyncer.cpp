#include "BlockSyncer.h"
#include "../ConnectionManager.h"
#include "../Messages/GetBlockMessage.h"

#include <BlockChainServer.h>
#include <Infrastructure/Logger.h>
#include <StringUtil.h>

BlockSyncer::BlockSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer)
	: m_connectionManager(connectionManager), m_blockChainServer(blockChainServer)
{
	m_timeout = std::chrono::system_clock::now();
	m_lastHeight = 0;
	m_highestRequested = 0;
}

bool BlockSyncer::SyncBlocks()
{
	const uint64_t height = m_blockChainServer.GetHeight(EChainType::CONFIRMED);
	const uint64_t highestHeight = m_connectionManager.GetHighestHeight();

	if (highestHeight >= (height + 5))
	{
		if (IsBlockSyncDue())
		{
			RequestBlocks();

			m_lastHeight = m_blockChainServer.GetHeight(EChainType::CONFIRMED);
			m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
		}

		return true;
	}

	return false;
}

bool BlockSyncer::IsBlockSyncDue()
{
	const uint64_t height = m_blockChainServer.GetHeight(EChainType::CONFIRMED);
	const uint64_t highestHeight = m_connectionManager.GetHighestHeight();

	if (highestHeight >= (height + 5))
	{
		// Check if block download timed out.
		if (m_timeout < std::chrono::system_clock::now())
		{
			LoggerAPI::LogWarning("BlockSyncer::IsBlockSyncDue() - Timed out. Requesting again.");
			return true;
		}

		// Check if blocks were received, and we're ready to request next batch.
		if ((height + 15) > m_highestRequested)
		{
			LoggerAPI::LogInfo("BlockSyncer::IsBlockSyncDue() - Blocks received. Requesting next batch.");
			return true;
		}

		// If progress was made, reset timeout.
		if (height >= (m_lastHeight + 5))
		{
			LoggerAPI::LogDebug("BlockSyncer::IsBlockSyncDue() - Blocks received. Requesting next batch.");
			m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
			m_lastHeight = height;
			return false;
		}
	}

	return false;
}

bool BlockSyncer::RequestBlocks()
{
	LoggerAPI::LogDebug("BlockSyncer::RequestBlocks - Requesting blocks.");

	std::vector<uint64_t> mostWorkPeers = m_connectionManager.GetMostWorkPeers();
	if (mostWorkPeers.empty())
	{
		LoggerAPI::LogWarning("BlockSyncer::RequestBlocks - No most-work peers found.");
		return false;
	}

	std::vector<std::pair<uint64_t, Hash>> blocksNeeded = m_blockChainServer.GetBlocksNeeded(10 * mostWorkPeers.size());
	if (blocksNeeded.empty())
	{
		LoggerAPI::LogWarning("BlockSyncer::RequestBlocks - No blocks needed.");
		return false;
	}

	size_t blocksRequested = 0;

	// If timed out, request full batch
	if (m_timeout < std::chrono::system_clock::now())
	{
		size_t nextPeer = 0;
		while (blocksRequested < blocksNeeded.size())
		{
			const GetBlockMessage getBlockMessage(blocksNeeded[blocksRequested].second);
			if (m_connectionManager.SendMessageToPeer(getBlockMessage, mostWorkPeers[nextPeer]))
			{
				++blocksRequested;
			}

			nextPeer = (nextPeer + 1) % mostWorkPeers.size();
		}

		m_highestRequested = blocksNeeded.back().first;
	}
	else
	{
		size_t nextPeer = 0;
		size_t blockIndex = 0;
		while (blockIndex < blocksNeeded.size())
		{
			auto& blockNeeded = blocksNeeded[blockIndex];
			if (blockNeeded.first <= m_highestRequested)
			{
				++blockIndex;
				continue;
			}

			const GetBlockMessage getBlockMessage(blockNeeded.second);
			if (m_connectionManager.SendMessageToPeer(getBlockMessage, mostWorkPeers[nextPeer]))
			{
				++blockIndex;
				++blocksRequested;
			}

			nextPeer = (nextPeer + 1) % mostWorkPeers.size();
		}

		m_highestRequested = blocksNeeded.back().first;
	}

	LoggerAPI::LogInfo(StringUtil::Format("BlockSyncer::RequestBlocks - %llu blocks requested from %llu peers.", blocksRequested, mostWorkPeers.size()));

	return true;
}