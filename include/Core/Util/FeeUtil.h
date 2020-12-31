#pragma once

#include <Core/Global.h>
#include <Core/Models/Transaction.h>
#include <Consensus/HardForks.h>
#include <Consensus/BlockWeight.h>

#include <algorithm>
#include <cstdint>

class FeeUtil
{
public:
	static uint64_t CalculateFee(const uint64_t feeBase, const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
	{
		return feeBase * Consensus::CalculateWeightV4(numInputs, numOutputs, numKernels);
	}
};