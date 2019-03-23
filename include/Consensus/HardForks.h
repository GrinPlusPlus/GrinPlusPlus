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
	// Fork every 250,000 blocks for first 2 years. Simple number and just a little less than 6 months.
	static const uint64_t HARD_FORK_INTERVAL = 250000;

	// Check whether the block version is valid at a given height. Implements 6 months interval scheduled hard forks for the first 2 years.
	static bool IsValidHeaderVersion(const uint64_t height, const uint16_t version)
	{
		if (height < HARD_FORK_INTERVAL)
		{
			return version == 1;
		}
		/*else if (height < HARD_FORK_INTERVAL)
		{
			return version == 2;
		}
		else if (height < 2 * HARD_FORK_INTERVAL)
		{
			return version == 3;
		} 
		else if (height < 3 * HARD_FORK_INTERVAL)
		{
			return version == 4;
		}
		else
		{
			return version == 5;
		}*/

		return false;
	}
}