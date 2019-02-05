#pragma once

#include <stdint.h>

class WalletUtil
{
public:
	static uint64_t CalculateFee(const uint64_t feeBase, const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels);

private:
	static uint64_t CalculateTxWeight(const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels);
};