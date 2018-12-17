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
	// Height for the v2 headers hard fork, with extended proof of work in header.
	static const uint64_t HEADER_V2_HARD_FORK = 95000;

	// Fork every 250,000 blocks for first 2 years. Simple number and just a little less than 6 months.
	static const uint64_t HARD_FORK_INTERVAL = 250000;

	// Check whether the block version is valid at a given height. Implements 6 months interval scheduled hard forks for the first 2 years.
	static uint16_t GetValidHeaderVersion(const uint64_t height)
	{
		if (height < HEADER_V2_HARD_FORK)
		{
			return 1;
		}
		else if (height < HARD_FORK_INTERVAL)
		{
			return 2;
		}
		else if (height < 2 * HARD_FORK_INTERVAL)
		{
			return 3;
		} 
		else if (height < 3 * HARD_FORK_INTERVAL)
		{
			return 4;
		}
		else
		{
			return 5;
		}
	}
}