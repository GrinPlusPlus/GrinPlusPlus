#include "PoWValidator.h"
#include "../uint128/uint128_t.h"

#include <Consensus/BlockTime.h>
#include <Consensus/BlockDifficulty.h>

bool PoWValidator::IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	const ProofOfWork& proofOfWork = header.GetProofOfWork();
	const ProofOfWork& previousProofOfWork = previousHeader.GetProofOfWork();

	// Validate Total Difficulty
	if (proofOfWork.GetTotalDifficulty() <= previousProofOfWork.GetTotalDifficulty())
	{
		return false;
	}

	const uint64_t targetDifficulty = proofOfWork.GetTotalDifficulty() - previousProofOfWork.GetTotalDifficulty();
	if (GetMaximumDifficulty(proofOfWork) < targetDifficulty)
	{
		return false;
	}

	// TODO: Implement
	//let child_batch = ctx.batch.child() ? ;
	//let diff_iter = store::DifficultyIter::from_batch(header.previous, child_batch);
	//let next_header_info = Consensus::next_difficulty(header.GetHeight(), diff_iter);
	//if (targetDifficulty != next_header_info.difficulty
	//{
	//	return Err(ErrorKind::WrongTotalDifficulty.into());
	//}

	// TODO: Implement
	// check the secondary PoW scaling factor if applicable
	//if (proofOfWork.GetScalingDifficulty() != next_header_info.secondary_scaling)
	//{
	//	return Err(ErrorKind::InvalidScaling.into());
	//}

	// TODO: Implement Siphash verification

	return true;
}

// Maximum difficulty this proof of work can achieve
uint64_t PoWValidator::GetMaximumDifficulty(const ProofOfWork& proofOfWork) const
{
	uint128_t scalingDifficulty = 0;

	// 2 proof of works, Cuckoo29 (for now) and Cuckoo30+, which are scaled differently (scaling not controlled for now).
	if (proofOfWork.GetEdgeBits() == Consensus::SECOND_POW_EDGE_BITS)
	{
		scalingDifficulty = proofOfWork.GetScalingDifficulty();
	}
	else
	{
		scalingDifficulty = (((uint64_t)2) << ((uint64_t)proofOfWork.GetEdgeBits() - Consensus::BASE_EDGE_BITS)) * ((uint64_t)proofOfWork.GetEdgeBits());
	}

	std::vector<unsigned char> temp;
	temp.resize(sizeof(uint64_t));
	std::reverse_copy(proofOfWork.Hash().GetData().cbegin(), proofOfWork.Hash().GetData().cbegin() + sizeof(uint64_t), temp.begin());

	uint64_t hash64;
	memcpy(&hash64, &temp[0], sizeof(uint64_t));

	uint128_t scaled = scalingDifficulty << 64;
	uint128_t hash128 = (uint128_t)hash64;
	const uint128_t difference = scaled / hash128;

	if (difference < UINT64_MAX)
	{
		return (uint64_t)difference;
	}

	return UINT64_MAX;
}