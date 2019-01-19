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

bool BlockSyncer::SyncBlocks(const SyncStatus& syncStatus)
{
	const uint64_t height = syncStatus.GetBlockHeight();
	const uint64_t highestHeight = m_connectionManager.GetHighestHeight();

	if (highestHeight >= (height + 5))
	{
		if (IsBlockSyncDue(syncStatus))
		{
			RequestBlocks();

			m_lastHeight = syncStatus.GetBlockHeight();
			m_timeout = std::chrono::system_clock::now() + std::chrono::seconds(5);
		}

		return true;
	}

	return false;
}

bool BlockSyncer::IsBlockSyncDue(const SyncStatus& syncStatus)
{
	const uint64_t chainHeight = syncStatus.GetBlockHeight();

	const uint64_t blocksDownloaded = (chainHeight - m_lastHeight);
	if (blocksDownloaded > 0)
	{
		LoggerAPI::LogDebug(StringUtil::Format("BlockSyncer::IsBlockSyncDue() - %llu blocks received since last check.", blocksDownloaded));
	}

	// Remove downloaded blocks from queue.
	for (size_t i = m_lastHeight; i <= chainHeight; i++)
	{
		m_requestedBlocks.erase(i);
	}

	m_lastHeight = chainHeight;

	// Check if blocks were received, and we're ready to request next batch.
	if (m_requestedBlocks.size() < 15)
	{
		LoggerAPI::LogInfo("BlockSyncer::IsBlockSyncDue() - Requesting next batch.");
		return true;
	}

	// Make sure we have valid requests for the first 15 blocks.
	std::vector<std::pair<uint64_t, Hash>> blocksNeeded = m_blockChainServer.GetBlocksNeeded(15);
	uint64_t index = 0;
	for (auto iter = blocksNeeded.cbegin(); iter != blocksNeeded.cend(); iter++)
	{
		auto requestedBlocksIter = m_requestedBlocks.find(iter->first);
		if (requestedBlocksIter == m_requestedBlocks.end())
		{
			LoggerAPI::LogInfo("BlockSyncer::IsBlockSyncDue() - Block not requested yet. Requesting now.");
			return true;
		}

		if (m_slowPeers.count(requestedBlocksIter->second.PEER_ID) > 0)
		{
			LoggerAPI::LogInfo("BlockSyncer::IsBlockSyncDue() - Waiting on block from banned peer. Requesting from valid peer now.");
			return true;
		}

		if (requestedBlocksIter->second.TIMEOUT < std::chrono::system_clock::now())
		{
			if (m_connectionManager.GetPipeline().IsProcessingBlock(iter->second))
			{
				break;
			}

			LoggerAPI::LogWarning("BlockSyncer::IsBlockSyncDue() - Block request timed out. Requesting again.");
			return true;
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

	std::vector<std::pair<uint64_t, Hash>> blocksNeeded = m_blockChainServer.GetBlocksNeeded(15 * mostWorkPeers.size());
	if (blocksNeeded.empty())
	{
		LoggerAPI::LogWarning("BlockSyncer::RequestBlocks - No blocks needed.");
		return false;
	}

	size_t blockIndex = 0;
	size_t blocksRequested = 0;

	size_t nextPeer = std::rand() % mostWorkPeers.size();
	while (blockIndex < blocksNeeded.size())
	{
		if (m_connectionManager.GetPipeline().IsProcessingBlock(blocksNeeded[blockIndex].second))
		{
			++blockIndex;
			continue;
		}

		bool blockRequested = false;
		auto iter = m_requestedBlocks.find(blocksNeeded[blockIndex].first);
		if (iter != m_requestedBlocks.end())
		{
			if (m_slowPeers.count(iter->second.PEER_ID) > 0 || iter->second.TIMEOUT < std::chrono::system_clock::now())
			{
				m_connectionManager.BanConnection(iter->second.PEER_ID);
				m_slowPeers.insert(iter->second.PEER_ID);

				const GetBlockMessage getBlockMessage(blocksNeeded[blockIndex].second);
				if (m_connectionManager.SendMessageToPeer(getBlockMessage, mostWorkPeers[nextPeer]))
				{
					iter->second.PEER_ID = nextPeer;
					iter->second.TIMEOUT = std::chrono::system_clock::now() + std::chrono::seconds(5);
					blockRequested = true;
					++blocksRequested;
					nextPeer = (nextPeer + 1) % mostWorkPeers.size();
					break;
				}
			}
		}

		if (!blockRequested)
		{
			const GetBlockMessage getBlockMessage(blocksNeeded[blockIndex].second);
			if (m_connectionManager.SendMessageToPeer(getBlockMessage, mostWorkPeers[nextPeer]))
			{
				RequestedBlock blockRequested;
				blockRequested.BLOCK_HEIGHT = blocksNeeded[blockIndex].first;
				blockRequested.PEER_ID = nextPeer;
				blockRequested.TIMEOUT = std::chrono::system_clock::now() + std::chrono::seconds(5);
				nextPeer = (nextPeer + 1) % mostWorkPeers.size();

				++blocksRequested;
				m_requestedBlocks[blockRequested.BLOCK_HEIGHT] = std::move(blockRequested);
			}
		}

		++blockIndex;
	}

	if (blocksRequested > 0)
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockSyncer::RequestBlocks - %llu blocks requested from %llu peers.", blocksRequested, mostWorkPeers.size()));
	}

	return true;
}