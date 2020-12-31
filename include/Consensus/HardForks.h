#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <Consensus/BlockTime.h>
#include <Config/EnvironmentType.h>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	// Fork every 6 months.
	static constexpr uint64_t HARD_FORK_INTERVAL = YEAR_HEIGHT / 2;

	// Floonet-only hardforks
	static const uint64_t FLOONET_FIRST_HARD_FORK = 185'040;
	static const uint64_t FLOONET_SECOND_HARD_FORK = 298'080;
	static const uint64_t FLOONET_THIRD_HARD_FORK = 552'960;
	static const uint64_t FLOONET_FOURTH_HARD_FORK = 642'240;

	static uint16_t GetHeaderVersion(const EEnvironmentType& environment, const uint64_t height)
	{
		if (environment == EEnvironmentType::FLOONET)
		{
			if (height < FLOONET_FIRST_HARD_FORK)
			{
				return 1;
			}
			else if (height < FLOONET_SECOND_HARD_FORK)
			{
				return 2;
			}
			else if (height < FLOONET_THIRD_HARD_FORK)
			{
				return 3;
			}
			else if (height < FLOONET_FOURTH_HARD_FORK)
			{
				return 4;
			}
		}
		else
		{
			if (height < HARD_FORK_INTERVAL)
			{
				return 1;
			}
			else if (height < 2 * HARD_FORK_INTERVAL)
			{
				return 2;
			}
			else if (height < 3 * HARD_FORK_INTERVAL)
			{
				return 3;
			}
			else if (height < 4 * HARD_FORK_INTERVAL)
			{
				return 4;
			}
		}

		return 5;
	}
}