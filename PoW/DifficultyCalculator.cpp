#include "DifficultyCalculator.h"
#include "DifficultyLoader.h"

#include <Consensus/BlockDifficulty.h>

using namespace Consensus;

DifficultyCalculator::DifficultyCalculator(const IBlockDB& blockDB)
	: m_blockDB(blockDB)
{

}

// Computes the proof-of-work difficulty that the next block should comply
// with. Takes an iterator over past block headers information, from latest
// (highest height) to oldest (lowest height).
//
// The difficulty calculation is based on both Digishield and GravityWave
// family of difficulty computation, coming to something very close to Zcash.
// The reference difficulty is an average of the difficulty over a window of
// DIFFICULTY_ADJUST_WINDOW blocks. The corresponding timespan is calculated
// by using the difference between the median timestamps at the beginning
// and the end of the window.
//
// The secondary proof-of-work factor is calculated along the same lines, as
// an adjustment on the deviation against the ideal value.
HeaderInfo DifficultyCalculator::CalculateNextDifficulty(const BlockHeader& header) const
{
	// Create vector of difficulty data running from earliest
	// to latest, and pad with simulated pre-genesis data to allow earlier
	// adjustment if there isn't enough window data length will be
	// DIFFICULTY_ADJUST_WINDOW + 1 (for initial block time bound)
	const std::vector<HeaderInfo> difficultyData = DifficultyLoader(m_blockDB).LoadDifficultyData(header);

	// First, get the ratio of secondary PoW vs primary, skipping initial header
	const std::vector<HeaderInfo> difficultyDataSkipFirst(difficultyData.cbegin() + 1, difficultyData.cend());
	const uint64_t sec_pow_scaling = SecondaryPOWScaling(header.GetHeight(), difficultyDataSkipFirst);

	// Get the timestamp delta across the window
	const uint64_t ts_delta = difficultyData[DIFFICULTY_ADJUST_WINDOW].GetTimestamp() - difficultyData[0].GetTimestamp();

	// Get the difficulty sum of the last DIFFICULTY_ADJUST_WINDOW elements
	uint64_t difficultySum = 0;
	for (const HeaderInfo& headerInfo : difficultyDataSkipFirst)
	{
		difficultySum += headerInfo.GetDifficulty();
	}

	const uint64_t actual = Damp(ts_delta, BLOCK_TIME_WINDOW, DAMP_FACTOR);

	// adjust time delta toward goal subject to dampening and clamping
	const uint64_t adj_ts = Clamp(actual, BLOCK_TIME_WINDOW, CLAMP_FACTOR);

	// minimum difficulty avoids getting stuck due to dampening
	const uint64_t difficulty = (std::max)(MIN_DIFFICULTY, difficultySum * BLOCK_TIME_SEC / adj_ts);

	return HeaderInfo::FromDiffAndScaling(difficulty, sec_pow_scaling);
}

// Count, in units of 1/100 (a percent), the number of "secondary" (AR) blocks in the provided window of blocks.
uint64_t DifficultyCalculator::ARCount(const std::vector<HeaderInfo>& difficultyData) const
{
	uint64_t numSecondary = 0;
	for (const HeaderInfo& headerInfo : difficultyData)
	{
		if (headerInfo.IsSecondary())
		{
			++numSecondary;
		}
	}

	return numSecondary * 100;
}

uint64_t DifficultyCalculator::ScalingFactorSum(const std::vector<HeaderInfo>& difficultyData) const
{
	uint64_t numSecondary = 0;
	for (const HeaderInfo& headerInfo : difficultyData)
	{
		numSecondary += headerInfo.GetSecondaryScaling();
	}

	return numSecondary;
}

// Factor by which the secondary proof of work difficulty will be adjusted
uint32_t DifficultyCalculator::SecondaryPOWScaling(const uint64_t height, const std::vector<HeaderInfo>& difficultyData) const
{
	// Get the scaling factor sum of the last DIFFICULTY_ADJUST_WINDOW elements
	const uint64_t scale_sum = ScalingFactorSum(difficultyData);

	// compute ideal 2nd_pow_fraction in pct and across window
	const uint64_t target_pct = Consensus::SecondaryPOWRatio(height);
	const uint64_t target_count = DIFFICULTY_ADJUST_WINDOW * target_pct;

	const uint64_t actual = Damp(ARCount(difficultyData), target_count, AR_SCALE_DAMP_FACTOR);

	// Get the secondary count across the window, adjusting count toward goal
	// subject to dampening and clamping.
	const uint64_t adj_count = Clamp(actual, target_count, CLAMP_FACTOR);
	const uint64_t scale = scale_sum * target_pct / (std::max)((uint64_t)1, adj_count);

	// minimum AR scale avoids getting stuck due to dampening
	return (uint32_t)(std::max)(MIN_AR_SCALE, scale);
}