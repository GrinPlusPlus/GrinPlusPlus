#pragma once

#include <Core/Models/Transaction.h>

#include <algorithm>
#include <cstdint>

class FeeUtil
{
public:
	static uint64_t CalculateActualFee(const Transaction& transaction)
	{
		uint64_t fee = 0;
		for (auto& kernel : transaction.GetKernels())
		{
			fee += kernel.GetFee();
		}

		return fee;
	}

	static uint64_t CalculateMinimumFee(const uint64_t feeBase, const Transaction& transaction)
	{
		const size_t numInputs = transaction.GetInputs().size();
		const size_t numOutputs = transaction.GetOutputs().size();
		const size_t numKernels = transaction.GetKernels().size();

		return CalculateFee(feeBase, (int64_t)numInputs, (int64_t)numOutputs, (int64_t)numKernels);
	}

	static uint64_t CalculateFee(const uint64_t feeBase, const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
	{
		return feeBase * CalculateTxWeight(numInputs, numOutputs, numKernels);
	}

private:
	static uint64_t CalculateTxWeight(const int64_t numInputs, const int64_t numOutputs, const int64_t numKernels)
	{
		return (std::max)((-1 * numInputs) + (4 * numOutputs) + (1 * numKernels), (int64_t)1);
	}
};