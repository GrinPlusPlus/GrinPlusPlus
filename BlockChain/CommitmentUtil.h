#pragma once

#include <stdint.h>
#include <vector>
#include <Core/FullBlock.h>
#include <Crypto/BlindingFactor.h>

class CommitmentUtil
{
public:
	static bool VerifyKernelSums(const FullBlock& block, const int64_t overage, const BlindingFactor& kernelOffset);
	static BlindingFactor AddKernelOffsets(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative);
};