#include "BlockLocator.h"

#include <P2P/Common.h>
#include <cmath>

std::vector<Hash> BlockLocator::GetLocators(const SyncStatus& syncStatus) const
{
	const std::vector<uint64_t> locatorHeights = GetLocatorHeights(syncStatus);

	std::vector<Hash> locators;
	locators.reserve(locatorHeights.size());
	for (const uint64_t locatorHeight : locatorHeights)
	{
		auto pHeader = m_pBlockChain->GetBlockHeaderByHeight(locatorHeight, EChainType::CANDIDATE);
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

std::vector<BlockHeaderPtr> BlockLocator::LocateHeaders(const std::vector<Hash>& locatorHashes) const
{
	std::vector<BlockHeaderPtr> blockHeaders;

	auto pCommonHeader = FindCommonHeader(locatorHashes);
	if (pCommonHeader != nullptr)
	{
		const uint64_t totalHeight = m_pBlockChain->GetHeight(EChainType::CANDIDATE);
		const uint64_t headerHeight = pCommonHeader->GetHeight();
		const uint64_t numHeadersToSend = (std::min)(totalHeight - headerHeight, (uint64_t)P2P::MAX_BLOCK_HEADERS);

		for (int i = 1; i <= numHeadersToSend; i++)
		{
			auto pHeader = m_pBlockChain->GetBlockHeaderByHeight(headerHeight + i, EChainType::CANDIDATE);
			if (pHeader == nullptr)
			{
				break;
			}

			blockHeaders.push_back(pHeader);
		}
	}
	
	return blockHeaders;
}

BlockHeaderPtr BlockLocator::FindCommonHeader(const std::vector<Hash>& locatorHashes) const
{
	for (const Hash& locatorHash : locatorHashes)
	{
		auto pHeader = m_pBlockChain->GetBlockHeaderByHash(locatorHash);
		if (pHeader != nullptr)
		{
			return pHeader;
		}
	}

	return nullptr;
}