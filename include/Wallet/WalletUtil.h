#pragma once

#include <Core/Models/Features.h>
#include <Core/Util/FeeUtil.h>
#include <Consensus/BlockTime.h>
#include <algorithm>
#include <stdint.h>

class WalletUtil
{
public:
	static uint64_t CalculateFee(
		const uint64_t feeBase,
		const uint64_t numInputs,
		const uint64_t numOutputs,
		const uint64_t numKernels)
	{
		return FeeUtil::CalculateFee(feeBase, (int64_t)numInputs, (int64_t)numOutputs, (int64_t)numKernels);
	}

	static bool IsOutputImmature(
		const EEnvironmentType environment,
		const EOutputFeatures features,
		const uint64_t outputBlockHeight,
		const uint64_t currentBlockHeight,
		const uint64_t minimumConfirmations)
	{
		if (features == EOutputFeatures::COINBASE_OUTPUT)
		{
			return Consensus::GetMaxCoinbaseHeight(environment, currentBlockHeight) <= outputBlockHeight;
		}
		else
		{
			return (outputBlockHeight + minimumConfirmations) > currentBlockHeight;
		}
	}
};