#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <algorithm>

#include <Consensus/BlockTime.h>
#include <Core/Global.h>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	// Cuckoo-cycle proof size (cycle length)
	static const uint8_t PROOFSIZE = 42;

	// Default Cuckoo Cycle size shift used for mining and validating.
	static const uint8_t DEFAULT_MIN_EDGE_BITS = 31;

	// Secondary proof-of-work size shift, meant to be ASIC resistant.
	static const uint8_t SECOND_POW_EDGE_BITS = 29;

	// Original reference edge_bits to compute difficulty factors for higher Cuckoo graph sizes, changing this would hard fork
	static const uint8_t BASE_EDGE_BITS = 24;

	// Clamp factor to use for difficulty adjustment
	// Limit value to within this factor of goal
	static const uint64_t CLAMP_FACTOR = 2;

	// Dampening factor to use for difficulty adjustment
	static const uint64_t DMA_DAMP_FACTOR = 3;

	// Dampening factor to use for AR scale calculation.
	static const uint64_t AR_SCALE_DAMP_FACTOR = 13;

	// Minimum scaling factor for AR pow, enforced in diff retargetting
	// avoids getting stuck when trying to increase ar_scale subject to dampening
	static const uint64_t MIN_AR_SCALE = AR_SCALE_DAMP_FACTOR;

	// Minimum difficulty, enforced in diff retargetting
	// avoids getting stuck when trying to increase difficulty subject to dampening
	static const uint64_t MIN_DMA_DIFFICULTY = DMA_DAMP_FACTOR;

	// Compute weight of a graph as number of siphash bits defining the graph
	// Must be made dependent on height to phase out smaller size over the years
	// This can wait until end of 2019 at latest
	static uint64_t GraphWeight(const uint64_t height, const uint8_t edge_bits)
	{
		const uint64_t expiry_height = YEAR_HEIGHT;

		uint64_t xpr_edge_bits = (uint64_t)edge_bits;
		if (edge_bits == 31 && height >= expiry_height)
		{
			xpr_edge_bits -= (std::min)(xpr_edge_bits, (uint64_t)(1ull + ((height - expiry_height) / WEEK_HEIGHT)));
		}

		return (2ull << ((uint64_t)(edge_bits - BASE_EDGE_BITS))) * xpr_edge_bits;
	}

	// Initial mining secondary scale
	static const uint32_t INITIAL_GRAPH_WEIGHT = (uint32_t)GraphWeight(0, SECOND_POW_EDGE_BITS);

	// Move value linearly toward a goal
	static uint64_t Damp(const uint64_t actual, const uint64_t goal, const uint64_t damp_factor)
	{
		return (actual + (damp_factor - 1) * goal) / damp_factor;
	}

	// limit value to be within some factor from a goal
	static uint64_t Clamp(const uint64_t actual, const uint64_t goal, const uint64_t clamp_factor)
	{
		return (std::max)(goal / clamp_factor, (std::min)(actual, goal * clamp_factor));
	}

	// Ratio the secondary proof of work should take over the primary, as a function of block height (time).
	// Starts at 90% losing a percent approximately every week. Represented as an integer between 0 and 100.
	static uint64_t SecondaryPOWRatio(const uint64_t height)
	{
		return 90 - (std::min)((uint64_t)90, (uint64_t)(height / (2 * YEAR_HEIGHT / 90)));
	}

	static uint64_t ScalingDifficulty(const uint64_t edgeBits)
	{
		return (((uint64_t)2) << (edgeBits - BASE_EDGE_BITS)) * edgeBits;
	}
	/// minimum solution difficulty after HardFork4 when PoW becomes primary only Cuckatoo32+
	static constexpr uint64_t C32_GRAPH_WEIGHT = (((uint64_t)2) << ((uint64_t)(32 - BASE_EDGE_BITS))) * 32; // 16384

	static uint64_t min_wtema_graph_weight()
	{
		const EEnvironmentType env = Global::GetEnv();
		if (env == EEnvironmentType::MAINNET) {
			return C32_GRAPH_WEIGHT;
		} else {
			return GraphWeight(0, SECOND_POW_EDGE_BITS);
		}
	}
}
