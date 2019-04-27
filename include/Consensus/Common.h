#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	// A grin is divisible to 10^9, following the SI prefixes
	static const uint64_t GRIN_BASE = 1000000000;
	// Milligrin, a thousand of a grin
	static const uint64_t MILLI_GRIN = GRIN_BASE / 1000;
	// Microgrin, a thousand of a milligrin
	static const uint64_t MICRO_GRIN = MILLI_GRIN / 1000;
	// Nanogrin, smallest unit, takes a billion to make a grin
	static const uint64_t NANO_GRIN = 1;

	// The block subsidy amount, one grin per second on average
	static const int64_t REWARD = 60 * GRIN_BASE;

	// Actual block reward for a given total fee amount
	static uint64_t CalculateReward(const uint64_t fee)
	{
		return REWARD + fee;
	}
}