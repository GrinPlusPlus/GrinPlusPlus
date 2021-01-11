#pragma once

#include <Consensus.h>

class FeeUtil
{
public:
	static uint64_t CalculateFee(const uint64_t feeBase, const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
	{
		return feeBase * Consensus::CalculateWeightV5(numInputs, numOutputs, numKernels);
	}
};