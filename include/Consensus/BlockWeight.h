#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <utility>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	// Weight of an input when counted against the max block weight capacity
	static const uint64_t BLOCK_INPUT_WEIGHT = 1;

	// Weight of an output when counted against the max block weight capacity
	static const uint64_t BLOCK_OUTPUT_WEIGHT = 21;

	// Weight of a kernel when counted against the max block weight capacity
	static const uint64_t BLOCK_KERNEL_WEIGHT = 3;


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
		return (num_inputs * BLOCK_INPUT_WEIGHT)
			+ (num_outputs * BLOCK_OUTPUT_WEIGHT)
			+ (num_kernels * BLOCK_KERNEL_WEIGHT);
	}
}