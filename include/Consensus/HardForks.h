#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <Consensus/BlockTime.h>
#include <Config/EnvironmentType.h>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	// Fork every 6 months.
	static constexpr uint64_t HARD_FORK_INTERVAL = YEAR_HEIGHT / 2;

	// Floonet-only first hardfork
	static const uint64_t FLOONET_FIRST_HARD_FORK = 185040;

	// Floonet-only second hardfork
	static const uint64_t FLOONET_SECOND_HARD_FORK = 298080;

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
			else if (height < 3 * HARD_FORK_INTERVAL)
			{
				return 3;
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
		}

		throw std::exception();
	}

	// Check whether the block version is valid at a given height. Implements 6 months interval scheduled hard forks for the first 2 years.
	static bool IsValidHeaderVersion(const EEnvironmentType& environment, const uint64_t height, const uint16_t version)
	{
		return GetHeaderVersion(environment, height) == version;
	}
}