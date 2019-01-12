#pragma once

#include <stdint.h>
#include <vector>
#include <Core/FullBlock.h>
#include <Crypto/BlindingFactor.h>

class CommitmentUtil
{
public:
	static bool VerifyKernelSums(const FullBlock& block, const int64_t overage, const BlindingFactor& kernelOffset);
};