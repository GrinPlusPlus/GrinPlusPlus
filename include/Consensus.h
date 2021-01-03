#pragma once

// Copyright (c) 2018-2020 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <chrono>
#include <cstdint>
#include <utility>
#include <type_traits>

#include <Core/Global.h>
#include <Core/Traits/Hashable.h>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	////////////////////////////////////////
	// COMMON
	////////////////////////////////////////

	// A grin is divisible to 10^9, following the SI prefixes
	static const uint64_t GRIN_BASE = 1000000000;

	// Milligrin, a thousand of a grin
	static constexpr uint64_t MILLI_GRIN = GRIN_BASE / 1000;

	// Microgrin, a thousand of a milligrin
	static constexpr uint64_t MICRO_GRIN = MILLI_GRIN / 1000;

	// Nanogrin, smallest unit, takes a billion to make a grin
	static const uint64_t NANO_GRIN = 1;

	// The block subsidy amount, one grin per second on average
	static constexpr int64_t REWARD = 60 * GRIN_BASE;




	////////////////////////////////////////
	// SORTING
	////////////////////////////////////////
	template <class T, typename = std::enable_if_t<std::is_base_of<Traits::IHashable, T>::value>>
	static bool IsSorted(const std::vector<T>& values)
	{
		for (auto iter = values.cbegin(); iter != values.cend(); iter++) {
			if (iter + 1 == values.cend()) {
				break;
			}

			if (iter->GetHash() > (iter + 1)->GetHash()) {
				return false;
			}
		}

		return true;
	}




	////////////////////////////////////////
	// WEIGHT
	////////////////////////////////////////

	// Weight of an input when counted against the max block weight capacity
	static const uint64_t INPUT_WEIGHT = 1;

	// Weight of an output when counted against the max block weight capacity
	static const uint64_t OUTPUT_WEIGHT = 21;

	// Weight of a kernel when counted against the max block weight capacity
	static const uint64_t KERNEL_WEIGHT = 3;

	// Total maximum block weight. At current sizes, this means a maximum
	// theoretical size of:
	// * `(674 + 33 + 1) * (40_000 / 21) = 1_348_571` for a block with only outputs
	// * `(1 + 8 + 8 + 33 + 64) * (40_000 / 3) = 1_520_000` for a block with only kernels
	// * `(1 + 33) * 40_000 = 1_360_000` for a block with only inputs
	//
	// Regardless of the relative numbers of inputs/outputs/kernels in a block the maximum
	// block size is around 1.5MB
	// For a block full of "average" txs (2 inputs, 2 outputs, 1 kernel) we have -
	// `(1 * 2) + (21 * 2) + (3 * 1) = 47` (weight per tx)
	// `40_000 / 47 = 851` (txs per block)
	//
	static const uint32_t MAX_BLOCK_WEIGHT = 40000;

	static uint64_t CalculateWeightV4(const int64_t num_inputs, const int64_t num_outputs, const int64_t numKernels)
	{
		return (std::max)((-1 * num_inputs) + (4 * num_outputs) + (1 * numKernels), (int64_t)1);
	}

	static uint64_t CalculateWeightV5(const uint64_t num_inputs, const uint64_t num_outputs, const uint64_t num_kernels)
	{
		return (num_inputs * INPUT_WEIGHT)
			+ (num_outputs * OUTPUT_WEIGHT)
			+ (num_kernels * KERNEL_WEIGHT);
	}




	////////////////////////////////////////
	// TIME
	////////////////////////////////////////

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

	static uint64_t GetMaxCoinbaseHeight(const Environment environment, const uint64_t blockHeight)
	{
		if (environment == Environment::AUTOMATED_TESTING) {
			return (std::max)(blockHeight, (uint64_t)25) - (uint64_t)25;
		}

		return (std::max)(blockHeight, Consensus::COINBASE_MATURITY) - Consensus::COINBASE_MATURITY;
	}

	static uint64_t GetMaxCoinbaseHeight(const uint64_t block_height)
	{
		return GetMaxCoinbaseHeight(Global::GetEnv(), block_height);
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




	////////////////////////////////////////
	// DIFFICULTY
	////////////////////////////////////////

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
		if (edge_bits == 31 && height >= expiry_height) {
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
		const Environment env = Global::GetEnv();
		if (env == Environment::MAINNET) {
			return C32_GRAPH_WEIGHT;
		} else {
			return GraphWeight(0, SECOND_POW_EDGE_BITS);
		}
	}




	////////////////////////////////////////
	// HARD FORKS
	////////////////////////////////////////

	// Fork every 6 months.
	static constexpr uint64_t HARD_FORK_INTERVAL = YEAR_HEIGHT / 2;

	// Floonet-only hardforks
	static const uint64_t FLOONET_FIRST_HARD_FORK = 185'040;
	static const uint64_t FLOONET_SECOND_HARD_FORK = 298'080;
	static const uint64_t FLOONET_THIRD_HARD_FORK = 552'960;
	static const uint64_t FLOONET_FOURTH_HARD_FORK = 642'240;

	static uint16_t GetHeaderVersion(const Environment& environment, const uint64_t height)
	{
		if (environment == Environment::FLOONET) {
			if (height < FLOONET_FIRST_HARD_FORK) {
				return 1;
			} else if (height < FLOONET_SECOND_HARD_FORK) {
				return 2;
			} else if (height < FLOONET_THIRD_HARD_FORK) {
				return 3;
			} else if (height < FLOONET_FOURTH_HARD_FORK) {
				return 4;
			}
		} else {
			if (height < HARD_FORK_INTERVAL) {
				return 1;
			} else if (height < 2 * HARD_FORK_INTERVAL) {
				return 2;
			} else if (height < 3 * HARD_FORK_INTERVAL) {
				return 3;
			} else if (height < 4 * HARD_FORK_INTERVAL) {
				return 4;
			}
		}

		return 5;
	}

	static uint16_t GetHeaderVersion(const uint64_t height)
	{
		return GetHeaderVersion(Global::GetEnv(), height);
	}
}