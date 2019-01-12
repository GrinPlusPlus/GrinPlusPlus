#include "PoWValidator.h"
#include "uint128/uint128_t.h"
#include "PoWUtil.h"
#include "Cuckaroo.h"
#include "Cuckatoo.h"

#include <Consensus/BlockTime.h>
#include <Consensus/BlockDifficulty.h>

PoWValidator::PoWValidator(const Config& config)
	: m_config(config)
{

}

bool PoWValidator::IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	// Validate Total Difficulty
	if (header.GetTotalDifficulty() <= previousHeader.GetTotalDifficulty())
	{
		return false;
	}

	const uint64_t targetDifficulty = header.GetTotalDifficulty() - previousHeader.GetTotalDifficulty();
	if (GetMaximumDifficulty(header) < targetDifficulty)
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

	const ProofOfWork& proofOfWork = header.GetProofOfWork();
	const EPoWType powType = PoWUtil(m_config).DeterminePoWType(proofOfWork.GetEdgeBits());
	if (powType == EPoWType::CUCKAROO)
	{
		if (!Cuckaroo::Validate(header))
		{
			return false;
		}
	}
	else if (powType == EPoWType::CUCKATOO)
	{
		if (!Cuckatoo::Validate(header))
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

// Maximum difficulty this proof of work can achieve
uint64_t PoWValidator::GetMaximumDifficulty(const BlockHeader& header) const
{
	uint128_t scalingDifficulty = 0;

	const ProofOfWork& proofOfWork = header.GetProofOfWork();

	// 2 proof of works, Cuckoo29 (for now) and Cuckoo30+, which are scaled differently (scaling not controlled for now).
	if (proofOfWork.GetEdgeBits() == Consensus::SECOND_POW_EDGE_BITS)
	{
		scalingDifficulty = header.GetScalingDifficulty();
	}
	else
	{
		scalingDifficulty = (((uint64_t)2) << ((uint64_t)proofOfWork.GetEdgeBits() - Consensus::BASE_EDGE_BITS)) * ((uint64_t)proofOfWork.GetEdgeBits());
	}

	std::vector<unsigned char> temp;
	temp.resize(sizeof(uint64_t));
	std::reverse_copy(proofOfWork.GetHash().GetData().cbegin(), proofOfWork.GetHash().GetData().cbegin() + sizeof(uint64_t), temp.begin());

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