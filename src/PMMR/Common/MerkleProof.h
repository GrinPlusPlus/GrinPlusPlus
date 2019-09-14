#pragma once

#include <Crypto/BigInteger.h>
#include <stdint.h>
#include <vector>

class MerkleProof
{
private:
	// The size of the MMR at the time the proof was created.
	const uint64_t m_mmrSize;

	// The sibling path from the leaf up to the final sibling hashing to the root.
	const std::vector<CBigInteger<32>> m_path;
};