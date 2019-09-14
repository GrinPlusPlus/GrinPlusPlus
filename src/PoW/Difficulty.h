#pragma once

#include <stdint.h>

struct Difficulty
{
	Difficulty(const uint64_t number)
		: m_number(number)
	{

	}

	/*
	// Difficulty of zero, which is invalid (no target can be
	// calculated from it) but very useful as a start for additions.
	pub fn zero() -> Difficulty {
		Difficulty { num: 0 }
	}

	// Difficulty of MIN_DIFFICULTY
	pub fn min() -> Difficulty {
		Difficulty {
			num: MIN_DIFFICULTY,
		}
	}

	// Difficulty unit, which is the graph weight of minimal graph
	pub fn unit() -> Difficulty {
		Difficulty {
			num: global::initial_graph_weight() as u64,
		}
	}

	// Convert a `u32` into a `Difficulty`
	pub fn from_num(num: u64) -> Difficulty {
		// can't have difficulty lower than 1
		Difficulty { num: max(num, 1) }
	}

	// Computes the difficulty from a hash. Divides the maximum target by the
	// provided hash and applies the Cuck(at)oo size adjustment factor (see
	// https://lists.launchpad.net/mimblewimble/msg00494.html).
	fn from_proof_adjusted(height: u64, proof: &Proof) -> Difficulty {
		// scale with natural scaling factor
		Difficulty::from_num(proof.scaled_difficulty(graph_weight(height, proof.edge_bits)))
	}

	// Same as `from_proof_adjusted` but instead of an adjustment based on
	// cycle size, scales based on a provided factor. Used by dual PoW system
	// to scale one PoW against the other.
	fn from_proof_scaled(proof: &Proof, scaling: u32) -> Difficulty {
		// Scaling between 2 proof of work algos
		Difficulty::from_num(proof.scaled_difficulty(scaling as u64))
	}

	// Converts the difficulty into a u64
	pub fn to_num(&self) -> u64 {
		self.num
	}
	*/
};