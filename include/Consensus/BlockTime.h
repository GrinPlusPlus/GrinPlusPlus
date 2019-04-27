#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	// Block interval, in seconds, the network will tune its next_target for. Note
	// that we may reduce this value in the future as we get more data on mining
	// with Cuckoo Cycle, networks improve and block propagation is optimized
	// (adjusting the reward accordingly).
	static const uint64_t BLOCK_TIME_SEC = 60;

	// Nominal height for standard time intervals, hour is 60 blocks
	static const uint64_t HOUR_HEIGHT = 3600 / BLOCK_TIME_SEC;

	// A day is 1440 blocks
	static const uint64_t DAY_HEIGHT = 24 * HOUR_HEIGHT;

	// A week is 10,080 blocks
	static const uint64_t WEEK_HEIGHT = 7 * DAY_HEIGHT;

	// A year is 524,160 blocks
	static const uint64_t YEAR_HEIGHT = 52 * WEEK_HEIGHT;

	// Number of blocks before a coinbase matures and can be spent
	// set to nominal number of block in one day (1440 with 1-minute blocks)
	static const uint64_t COINBASE_MATURITY = (24 * 60 * 60) / BLOCK_TIME_SEC;

	// Default number of blocks in the past when cross-block cut-through will start
	// happening. Needs to be long enough to not overlap with a long reorg.
	// Rationale behind the value is the longest bitcoin fork was about 30 blocks, so 5h. 
	// We add an order of magnitude to be safe and round to 7x24h of blocks to make it
	// easier to reason about.
	static const uint32_t CUT_THROUGH_HORIZON = WEEK_HEIGHT;

	// Default number of blocks in the past to determine the height where we request
	// a txhashset (and full blocks from). Needs to be long enough to not overlap with
	// a long reorg.
	// Rationale behind the value is the longest bitcoin fork was about 30 blocks, so 5h.
	// We add an order of magnitude to be safe and round to 2x24h of blocks to make it
	// easier to reason about.
	static const uint32_t STATE_SYNC_THRESHOLD = 2 * DAY_HEIGHT;

	// Time window in blocks to calculate block time median
	static const uint64_t MEDIAN_TIME_WINDOW = 11;

	// Index at half the desired median
	static const uint64_t MEDIAN_TIME_INDEX = MEDIAN_TIME_WINDOW / 2;

	// Number of blocks used to calculate difficulty adjustments
	static const uint64_t DIFFICULTY_ADJUST_WINDOW = HOUR_HEIGHT;

	// Average time span of the difficulty adjustment window
	static const uint64_t BLOCK_TIME_WINDOW = DIFFICULTY_ADJUST_WINDOW * BLOCK_TIME_SEC;

	// Maximum size time window used for difficulty adjustments
	static const uint64_t UPPER_TIME_BOUND = BLOCK_TIME_WINDOW * 2;

	// Minimum size time window used for difficulty adjustments
	static const uint64_t LOWER_TIME_BOUND = BLOCK_TIME_WINDOW / 2;
}