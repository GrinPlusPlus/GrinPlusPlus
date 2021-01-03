#include "PoWValidator.h"
#include "uint128/uint128_t.h"
#include "DifficultyCalculator.h"
#include "PoWUtil.h"
#include "Cuckaroo.h"
#include "Cuckarood.h"
#include "Cuckaroom.h"
#include "Cuckarooz.h"
#include "Cuckatoo.h"

#include <Consensus/BlockTime.h>
#include <Consensus/BlockDifficulty.h>

PoWValidator::PoWValidator(const Config& config, std::shared_ptr<const IBlockDB> pBlockDB)
	: m_config(config), m_pBlockDB(pBlockDB)
{

}

bool PoWValidator::IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	// Validate Total Difficulty
	if (header.GetTotalDifficulty() <= previousHeader.GetTotalDifficulty())
	{
		LOG_WARNING_F("Target difficulty too low for block {}", header);
		return false;
	}

	const uint64_t max_difficulty = GetMaximumDifficulty(header);
	const uint64_t targetDifficulty = header.GetTotalDifficulty() - previousHeader.GetTotalDifficulty();
	if (max_difficulty < targetDifficulty)
	{
		LOG_WARNING_F("Target difficulty {} too high for block {}. Max: {}", targetDifficulty, header, max_difficulty);
		return false;
	}

	// Explicit check to ensure total_difficulty has increased by exactly the _network_ difficulty of the previous block.
	const HeaderInfo nextHeaderInfo = DifficultyCalculator(m_pBlockDB).CalculateNextDifficulty(header);
	if (targetDifficulty != nextHeaderInfo.GetDifficulty())
	{
		LOG_WARNING_F("Target difficulty invalid for block {} with previous block {}", header, previousHeader);
		return false;
	}

	// Check the secondary PoW scaling factor if applicable.
	if (header.GetScalingDifficulty() != nextHeaderInfo.GetSecondaryScaling())
	{
		LOG_WARNING_F("Scaling difficulty invalid for block {}", header);
		return false;
	}

	const ProofOfWork& proofOfWork = header.GetProofOfWork();
	const EPoWType powType = PoWUtil(m_config).DeterminePoWType(header.GetVersion(), proofOfWork.GetEdgeBits());
	if (powType == EPoWType::CUCKAROO)
	{
		return Cuckaroo::Validate(header);
	}
	else if (powType == EPoWType::CUCKAROOD)
	{
		return Cuckarood::Validate(header);
	}
	else if (powType == EPoWType::CUCKAROOM)
	{
		return Cuckaroom::Validate(header);
	}
	else if (powType == EPoWType::CUCKAROOZ)
	{
		return Cuckarooz::Validate(header);
	}
	else if (powType == EPoWType::CUCKATOO)
	{
		return Cuckatoo::Validate(header);
	}
	else
	{
		return false;
	}
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
		scalingDifficulty = Consensus::ScalingDifficulty(proofOfWork.GetEdgeBits());
	}

	std::vector<unsigned char> temp;
	temp.resize(sizeof(uint64_t));
	std::reverse_copy(
		proofOfWork.GetHash().GetData().cbegin(),
		proofOfWork.GetHash().GetData().cbegin() + sizeof(uint64_t),
		temp.begin()
	);

	uint64_t hash64;
	memcpy(&hash64, &temp[0], sizeof(uint64_t));

	uint128_t scaled = scalingDifficulty << 64;
	uint128_t hash128 = (uint128_t)(std::max)((uint64_t)1ull, hash64);
	const uint128_t difference = scaled / hash128;

	if (difference < UINT64_MAX)
	{
		return (uint64_t)difference;
	}

	return UINT64_MAX;
}