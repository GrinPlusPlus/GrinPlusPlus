#include "WalletUtil.h"

#include <algorithm>

uint64_t WalletUtil::CalculateFee(const uint64_t feeBase, const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
{
	return feeBase * CalculateTxWeight(numInputs, numOutputs, numKernels);
}

uint64_t WalletUtil::CalculateTxWeight(const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
{
	return std::max((-1 * numInputs) + (4 * numOutputs) + (1 * numKernels), (int64_t)1);
}