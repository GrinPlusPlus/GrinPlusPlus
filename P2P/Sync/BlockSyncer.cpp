#include "BlockSyncer.h"
#include "../ConnectionManager.h"
#include "../Messages/GetBlockMessage.h"

#include <BlockChain/BlockChainServer.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/StringUtil.h>

BlockSyncer::BlockSyncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer)
	: m_connectionManager(connectionManager), m_blockChainServer(blockChainServer)
{
	m_timeout = std::chrono::system_clock::now();
	m_lastHeight = 0;
	m_highestRequested = 0;
}

bool BlockSyncer::SyncBlocks(const SyncStatus& syncStatus, const bool startup)
{
	const uint64_t chainHeight = syncStatus.GetBlockHeight();
	const uint64_t networkHeight = syncStatus.GetNetworkHeight();

	if (networkHeight >= (chainHeight + 5) || (startup && networkHeight > chainHeight))
	{
		if (IsBlockSyncDue(syncStatus))
		{
			RequestBlocks();

			m_lastHeight = chainHeight;
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
		LoggerAPI::LogTrace(StringUtil::Format("BlockSyncer::IsBlockSyncDue() - %llu blocks received since last check.", blocksDownloaded));
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
		LoggerAPI::LogTrace("BlockSyncer::IsBlockSyncDue() - Requesting next batch.");
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
			LoggerAPI::LogTrace("BlockSyncer::IsBlockSyncDue() - Block not requested yet. Requesting now.");
			return true;
		}

		if (m_slowPeers.count(requestedBlocksIter->second.PEER_ID) > 0)
		{
			LoggerAPI::LogTrace("BlockSyncer::IsBlockSyncDue() - Waiting on block from banned peer. Requesting from valid peer now.");
			return true;
		}

		if (requestedBlocksIter->second.TIMEOUT < std::chrono::system_clock::now())
		{
			if (m_connectionManager.GetPipeline().IsProcessingBlock(iter->second))
			{
				continue;
			}

			LoggerAPI::LogWarning("BlockSyncer::IsBlockSyncDue() - Block request timed out. Requesting again.");
			return true;
		}
	}

	return false;
}

bool BlockSyncer::RequestBlocks()
{
	LoggerAPI::LogTrace("BlockSyncer::RequestBlocks - Requesting blocks.");

	std::vector<uint64_t> mostWorkPeers = m_connectionManager.GetMostWorkPeers();
	const uint64_t numPeers = mostWorkPeers.size();
	if (mostWorkPeers.empty())
	{
		LoggerAPI::LogDebug("BlockSyncer::RequestBlocks - No most-work peers found.");
		return false;
	}

	const uint64_t numBlocksNeeded = 16 * numPeers;
	std::vector<std::pair<uint64_t, Hash>> blocksNeeded = m_blockChainServer.GetBlocksNeeded(2 * numBlocksNeeded);
	if (blocksNeeded.empty())
	{
		LoggerAPI::LogTrace("BlockSyncer::RequestBlocks - No blocks needed.");
		return false;
	}

	size_t blockIndex = 0;

	std::vector<std::pair<uint64_t, Hash>> blocksToRequest;

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
				m_connectionManager.BanConnection(iter->second.PEER_ID, EBanReason::FraudHeight);
				m_slowPeers.insert(iter->second.PEER_ID);

				blocksToRequest.emplace_back(blocksNeeded[blockIndex]);
			}
		}
		else
		{
			blocksToRequest.emplace_back(blocksNeeded[blockIndex]);
		}

		if (blocksToRequest.size() >= numBlocksNeeded)
		{
			break;
		}

		++blockIndex;
	}

	size_t nextPeer = std::rand() % mostWorkPeers.size();
	for (size_t i = 0; i < blocksToRequest.size(); i++)
	{
		const GetBlockMessage getBlockMessage(blocksToRequest[i].second);
		if (m_connectionManager.SendMessageToPeer(getBlockMessage, mostWorkPeers[nextPeer]))
		{
			RequestedBlock blockRequested;
			blockRequested.BLOCK_HEIGHT = blocksToRequest[i].first;
			blockRequested.PEER_ID = mostWorkPeers[nextPeer];
			blockRequested.TIMEOUT = std::chrono::system_clock::now() + std::chrono::seconds(10);

			m_requestedBlocks[blockRequested.BLOCK_HEIGHT] = std::move(blockRequested);
		}

		if ((i + 1) % 16 == 0)
		{
			nextPeer = (nextPeer + 1) % mostWorkPeers.size();
		}
	}

	if (!blocksToRequest.empty())
	{
		LoggerAPI::LogTrace(StringUtil::Format("BlockSyncer::RequestBlocks - %llu blocks requested from %llu peers.", blocksToRequest.size(), numPeers));
	}

	return true;
}