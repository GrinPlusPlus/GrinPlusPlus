#include "BlockLocator.h"

#include <P2P/Common.h>
#include <cmath>

// TODO: Evaluate whether we should be using SYNC or CANDIDATE in this class.

BlockLocator::BlockLocator(IBlockChainServer& blockChainServer)
	: m_blockChainServer(blockChainServer)
{

}

std::vector<CBigInteger<32>> BlockLocator::GetLocators(const SyncStatus& syncStatus) const
{
	const std::vector<uint64_t> locatorHeights = GetLocatorHeights(syncStatus);

	std::vector<CBigInteger<32>> locators;
	locators.reserve(locatorHeights.size());
	for (const uint64_t locatorHeight : locatorHeights)
	{
		std::unique_ptr<BlockHeader> pHeader = m_blockChainServer.GetBlockHeaderByHeight(locatorHeight, EChainType::SYNC);
		if (pHeader != nullptr)
		{
			locators.push_back(pHeader->GetHash());
		}
	}

	return locators;
}

// current height back to 0 decreasing in powers of 2
std::vector<uint64_t> BlockLocator::GetLocatorHeights(const SyncStatus& syncStatus) const
{
	uint64_t current = syncStatus.GetHeaderHeight();
	
	std::vector<uint64_t> heights;
	while (current > 0)
	{
		heights.push_back(current);
		if (heights.size() >= (P2P::MAX_LOCATORS - 1))
		{
			break;
		}

		const uint64_t next = (uint64_t)std::pow(2, (uint32_t)heights.size());
		if (current > next)
		{ 
			current -= next;
		}
		else 
		{ 
			break;
		}
	}

	heights.push_back(0);
	
	return heights;
}

std::vector<BlockHeader> BlockLocator::LocateHeaders(const std::vector<CBigInteger<32>>& locatorHashes) const
{
	std::vector<BlockHeader> blockHeaders;

	std::unique_ptr<BlockHeader> pCommonHeader = FindCommonHeader(locatorHashes);
	if (pCommonHeader != nullptr)
	{
		const uint64_t totalHeight = m_blockChainServer.GetHeight(EChainType::SYNC);
		const uint64_t headerHeight = pCommonHeader->GetHeight();
		const uint64_t numHeadersToSend = std::min(totalHeight - headerHeight, (uint64_t)P2P::MAX_BLOCK_HEADERS);

		for (int i = 1; i <= numHeadersToSend; i++)
		{
			std::unique_ptr<BlockHeader> pHeader = m_blockChainServer.GetBlockHeaderByHeight(headerHeight + i, EChainType::SYNC);
			if (pHeader == nullptr)
			{
				break;
			}

			blockHeaders.push_back(*pHeader);
		}
	}
	
	return blockHeaders;
}

std::unique_ptr<BlockHeader> BlockLocator::FindCommonHeader(const std::vector<CBigInteger<32>>& locatorHashes) const
{
	for (CBigInteger<32> locatorHash : locatorHashes)
	{
		std::unique_ptr<BlockHeader> pHeader = m_blockChainServer.GetBlockHeaderByHash(locatorHash);
		if (pHeader != nullptr)
		{
			return pHeader;
		}
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}