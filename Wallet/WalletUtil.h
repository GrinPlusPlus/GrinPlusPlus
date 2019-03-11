#pragma once

#include <Core/Models/Features.h>
#include <stdint.h>

class WalletUtil
{
public:
	static uint64_t CalculateFee(const uint64_t feeBase, const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels);
	static bool IsOutputImmature(const EOutputFeatures features, const uint64_t outputBlockHeight, const uint64_t currentBlockHeight, const uint64_t minimumConfirmations);

private:
	static uint64_t CalculateTxWeight(const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels);
};