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
	// Cuckoo-cycle proof size (cycle length)
	static const uint8_t PROOFSIZE = 42;

	// Default Cuckoo Cycle size shift used for mining and validating.
	static const uint8_t DEFAULT_MIN_EDGE_BITS = 30;

	// Secondary proof-of-work size shift, meant to be ASIC resistant.
	static const uint8_t SECOND_POW_EDGE_BITS = 29;

	// Original reference edge_bits to compute difficulty factors for higher Cuckoo graph sizes, changing this would hard fork
	static const uint8_t BASE_EDGE_BITS = 24;

	// Original reference sizeshift to compute difficulty factors for higher
	// Cuckoo graph sizes, changing this would hard fork
	static const uint8_t REFERENCE_SIZESHIFT = 30;

	// Default Cuckoo Cycle easiness, high enough to have good likeliness to find a solution.
	static const uint32_t EASINESS = 50;

	// Dampening factor to use for difficulty adjustment
	static const uint64_t DAMP_FACTOR = 3;

	// The initial difficulty at launch. This should be over-estimated
	// and difficulty should come down at launch rather than up
	// Currently grossly over-estimated at 10% of current
	// ethereum GPUs (assuming 1GPU can solve a block at diff 1
	// in one block interval)
	static const uint64_t INITIAL_DIFFICULTY = 1000000;

	// Computes the proof-of-work difficulty that the next block should comply
	// with. Takes an iterator over past blocks, from latest (highest height) to
	// oldest (lowest height). The iterator produces pairs of timestamp and
	// difficulty for each block.
	//
	// The difficulty calculation is based on both Digishield and GravityWave
	// family of difficulty computation, coming to something very close to Zcash.
	// The reference difficulty is an average of the difficulty over a window of
	// DIFFICULTY_ADJUST_WINDOW blocks. The corresponding timespan is calculated
	// by using the difference between the median timestamps at the beginning
	// and the end of the window.
	// TODO: Implement this
	/*pub fn next_difficulty<T>(cursor: T)->Result<Difficulty, TargetError>
		where
		T: IntoIterator<Item = Result<(u64, Difficulty), TargetError>>,
	{
		// Create vector of difficulty data running from earliest
		// to latest, and pad with simulated pre-genesis data to allow earlier
		// adjustment if there isn't enough window data
		// length will be DIFFICULTY_ADJUST_WINDOW+MEDIAN_TIME_WINDOW
		let diff_data = global::difficulty_data_to_vector(cursor);

		// Obtain the median window for the earlier time period
		// the first MEDIAN_TIME_WINDOW elements
		let mut window_earliest : Vec<u64> = diff_data
			.iter()
			.take(MEDIAN_TIME_WINDOW as usize)
			.map(| n | n.clone().unwrap().0)
			.collect();
		// pick median
		window_earliest.sort();
		let earliest_ts = window_earliest[MEDIAN_TIME_INDEX as usize];

		// Obtain the median window for the latest time period
		// i.e. the last MEDIAN_TIME_WINDOW elements
		let mut window_latest : Vec<u64> = diff_data
			.iter()
			.skip(DIFFICULTY_ADJUST_WINDOW as usize)
			.map(| n | n.clone().unwrap().0)
			.collect();
		// pick median
		window_latest.sort();
		let latest_ts = window_latest[MEDIAN_TIME_INDEX as usize];

		// median time delta
		let ts_delta = latest_ts - earliest_ts;

		// Get the difficulty sum of the last DIFFICULTY_ADJUST_WINDOW elements
		let diff_sum = diff_data
			.iter()
			.skip(MEDIAN_TIME_WINDOW as usize)
			.fold(0, | sum, d | sum + d.clone().unwrap().1.to_num());

		// Apply dampening except when difficulty is near 1
		if (diff_sum < DAMP_FACTOR * DIFFICULTY_ADJUST_WINDOW)
		{
			ts_damp = ts_delta;
		}
		else 
		{
			ts_damp = (1 * ts_delta + (DAMP_FACTOR - 1) * BLOCK_TIME_WINDOW) / DAMP_FACTOR;
		}

		// Apply time bounds
		if (ts_damp < LOWER_TIME_BOUND)
		{
			adj_ts = LOWER_TIME_BOUND;
		}
		else if (ts_damp > UPPER_TIME_BOUND)
		{
			adj_ts = UPPER_TIME_BOUND;
		}
		else
		{
			adj_ts = ts_damp;
		}

		let difficulty = diff_sum * BLOCK_TIME_SEC / adj_ts;

		Ok(Difficulty::from_num(max(difficulty, 1)))
	}*/
}