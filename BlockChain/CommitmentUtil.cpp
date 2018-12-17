#include "CommitmentUtil.h"

bool CommitmentUtil::VerifyKernelSums(const FullBlock& block, const int64_t overage, const BlindingFactor& kernelOffset)
{
	// TODO: Implement
	return true;
}

BlindingFactor CommitmentUtil::AddKernelOffsets(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative)
{
	// TODO: Implement
	return BlindingFactor(CBigInteger<32>());
}