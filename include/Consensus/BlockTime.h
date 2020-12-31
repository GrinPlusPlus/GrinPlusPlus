#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <algorithm>
#include <chrono>

#include <Config/EnvironmentType.h>

#pragma warning(disable:4505)

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	// Block interval, in seconds, the network will tune its next_target for. Note
	// that we may reduce this value in the future as we get more data on mining
	// with Cuckoo Cycle, networks improve and block propagation is optimized
	// (adjusting the reward accordingly).
	static const uint64_t BLOCK_TIME_SEC = 60;

	// Nominal height for standard time intervals, hour is 60 blocks
	static constexpr uint64_t HOUR_HEIGHT = 3600 / BLOCK_TIME_SEC;

	// A day is 1440 blocks
	static constexpr uint64_t DAY_HEIGHT = 24 * HOUR_HEIGHT;

	// A week is 10,080 blocks
	static constexpr uint64_t WEEK_HEIGHT = 7 * DAY_HEIGHT;

	// A year is 524,160 blocks
	static constexpr uint64_t YEAR_HEIGHT = 52 * WEEK_HEIGHT;

	// Number of blocks before a coinbase matures and can be spent
	// set to nominal number of block in one day (1440 with 1-minute blocks)
	static constexpr uint64_t COINBASE_MATURITY = (24 * 60 * 60) / BLOCK_TIME_SEC;

	static uint64_t GetMaxCoinbaseHeight(const EEnvironmentType environment, const uint64_t blockHeight)
	{
		if (environment == EEnvironmentType::AUTOMATED_TESTING)
		{
			return (std::max)(blockHeight, (uint64_t)25) - (uint64_t)25;
		}

		return (std::max)(blockHeight, Consensus::COINBASE_MATURITY) - Consensus::COINBASE_MATURITY;
	}

	// Default number of blocks in the past when cross-block cut-through will start
	// happening. Needs to be long enough to not overlap with a long reorg.
	// Rationale behind the value is the longest bitcoin fork was about 30 blocks, so 5h. 
	// We add an order of magnitude to be safe and round to 7x24h of blocks to make it
	// easier to reason about.
	static const uint32_t CUT_THROUGH_HORIZON = WEEK_HEIGHT;

	static uint64_t GetHorizonHeight(const uint64_t blockHeight)
	{
		return (std::max)(blockHeight, (uint64_t)Consensus::CUT_THROUGH_HORIZON) - Consensus::CUT_THROUGH_HORIZON;
	}

	// Default number of blocks in the past to determine the height where we request
	// a txhashset (and full blocks from). Needs to be long enough to not overlap with
	// a long reorg.
	// Rationale behind the value is the longest bitcoin fork was about 30 blocks, so 5h.
	// We add an order of magnitude to be safe and round to 2x24h of blocks to make it
	// easier to reason about.
	static constexpr uint32_t STATE_SYNC_THRESHOLD = 2 * DAY_HEIGHT;

	// Time window in blocks to calculate block time median
	static const uint64_t MEDIAN_TIME_WINDOW = 11;

	// Index at half the desired median
	static constexpr uint64_t MEDIAN_TIME_INDEX = MEDIAN_TIME_WINDOW / 2;

	// Number of blocks used to calculate difficulty adjustments
	static const uint64_t DIFFICULTY_ADJUST_WINDOW = HOUR_HEIGHT;

	// Average time span of the difficulty adjustment window
	static constexpr uint64_t BLOCK_TIME_WINDOW = DIFFICULTY_ADJUST_WINDOW * BLOCK_TIME_SEC;

	// Maximum size time window used for difficulty adjustments
	static constexpr uint64_t UPPER_TIME_BOUND = BLOCK_TIME_WINDOW * 2;

	// Minimum size time window used for difficulty adjustments
	static constexpr uint64_t LOWER_TIME_BOUND = BLOCK_TIME_WINDOW / 2;

	/// default Future Time Limit (FTL) of 5 minutes
	static constexpr uint64_t DEFAULT_FUTURE_TIME_LIMIT_SEC = 5 * BLOCK_TIME_SEC;

	// Refuse blocks more than 12 block intervals in the future.
	static int64_t GetMaxBlockTime(const std::chrono::time_point<std::chrono::system_clock>& currentTime)
	{
		auto maxBlockTime = currentTime + std::chrono::seconds(DEFAULT_FUTURE_TIME_LIMIT_SEC);
		return maxBlockTime.time_since_epoch().count();
	}

	/// Difficulty adjustment half life (actually, 60s * number of 0s-blocks to raise diff by factor e) is 4 hours
	static constexpr uint64_t WTEMA_HALF_LIFE = 4 * 3600;
}