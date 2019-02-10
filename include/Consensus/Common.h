#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

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

	/*
	TODO: Implement this.
	// Consensus rule that collections of items are sorted lexicographically.
	pub trait VerifySortOrder<T> {
		// Verify a collection of items is sorted as required.
		fn verify_sort_order(&self)->Result<(), Error>;
	}
	*/
}