#pragma once

#include <Core/Models/Features.h>
#include <Consensus/BlockTime.h>
#include <algorithm>
#include <stdint.h>

class WalletUtil
{
public:
	static uint64_t CalculateFee(const uint64_t feeBase, const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
	{
		return feeBase * CalculateTxWeight(numInputs, numOutputs, numKernels);
	}

	static bool IsOutputImmature(const EOutputFeatures features, const uint64_t outputBlockHeight, const uint64_t currentBlockHeight, const uint64_t minimumConfirmations)
	{
		if (features == COINBASE_OUTPUT)
		{
			return (outputBlockHeight + Consensus::COINBASE_MATURITY) >= currentBlockHeight;
		}
		else
		{
			return (outputBlockHeight + minimumConfirmations) >= currentBlockHeight;
		}
	}

private:
	static uint64_t CalculateTxWeight(const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
	{
		return (std::max)((-1 * numInputs) + (4 * numOutputs) + (1 * numKernels), (int64_t)1);
	}
};