#include "DifficultyLoader.h"

#include <Consensus/BlockDifficulty.h>
#include <lru/cache.hpp>

// TODO: A FIFO cache would be more appropriate.
LRU::Cache<Hash, BlockHeader> BLOCK_HEADERS_CACHE(128);

DifficultyLoader::DifficultyLoader(const IBlockDB& blockDB)
	: m_blockDB(blockDB)
{

}

std::vector<HeaderInfo> DifficultyLoader::LoadDifficultyData(const BlockHeader& header) const
{
	const size_t numBlocksNeeded = Consensus::DIFFICULTY_ADJUST_WINDOW + 1;
	std::vector<HeaderInfo> difficultyData;
	difficultyData.reserve(numBlocksNeeded);

	std::unique_ptr<BlockHeader> pHeader = LoadHeader(header.GetPreviousBlockHash());
	while (difficultyData.size() < numBlocksNeeded && pHeader != nullptr)
	{
		const int64_t timestamp = pHeader->GetTimestamp();
		const uint64_t totalDifficulty = pHeader->GetTotalDifficulty();
		const uint32_t scalingFactor = pHeader->GetScalingDifficulty();
		const bool secondary = pHeader->GetProofOfWork().IsSecondary();

		pHeader = LoadHeader(pHeader->GetPreviousBlockHash());
		if (pHeader != nullptr)
		{
			const uint64_t difficulty = totalDifficulty - pHeader->GetTotalDifficulty();

			difficultyData.emplace_back(HeaderInfo(timestamp, difficulty, scalingFactor, secondary));
		}
		else
		{
			difficultyData.emplace_back(HeaderInfo(timestamp, totalDifficulty, scalingFactor, secondary));
		}
	}

	BLOCK_HEADERS_CACHE.insert(header.GetHash(), header);

	return PadDifficultyData(difficultyData);
}

std::unique_ptr<BlockHeader> DifficultyLoader::LoadHeader(const Hash& headerHash) const
{
	auto iter = BLOCK_HEADERS_CACHE.find(headerHash);
	if (iter != BLOCK_HEADERS_CACHE.end())
	{
		return std::make_unique<BlockHeader>(iter->second);
	}
	else
	{
		std::unique_ptr<BlockHeader> pHeader = m_blockDB.GetBlockHeader(headerHash);
		if (pHeader != nullptr)
		{
			BLOCK_HEADERS_CACHE.insert(headerHash, *pHeader);
		}

		return pHeader;
	}
}

// Converts an iterator of block difficulty data to more a more manageable
// vector and pads if needed (which will) only be needed for the first few
// blocks after genesis
std::vector<HeaderInfo> DifficultyLoader::PadDifficultyData(std::vector<HeaderInfo>& difficultyData) const
{
	// Only needed just after blockchain launch... basically ensures there's
	// always enough data by simulating perfectly timed pre-genesis
	// blocks at the genesis difficulty as needed.
	const size_t numBlocksNeeded = Consensus::DIFFICULTY_ADJUST_WINDOW + 1;
	const size_t size = difficultyData.size();
	if (numBlocksNeeded > size)
	{
		uint64_t last_ts_delta = Consensus::BLOCK_TIME_SEC;
		if (size > 1)
		{
			last_ts_delta = difficultyData[0].GetTimestamp() - difficultyData[1].GetTimestamp();
		}

		const uint64_t last_diff = difficultyData[0].GetDifficulty();

		// fill in simulated blocks with values from the previous real block
		uint64_t last_ts = difficultyData.back().GetTimestamp();
		while (difficultyData.size() < numBlocksNeeded)
		{
			last_ts -= std::min(last_ts, last_ts_delta);
			difficultyData.emplace_back(HeaderInfo::FromTimeAndDiff(last_ts, last_diff));
		}
	}

	std::reverse(difficultyData.begin(), difficultyData.end());

	return difficultyData;
}