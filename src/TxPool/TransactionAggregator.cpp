#include "TransactionAggregator.h"

#include <Core/Util/TransactionUtil.h>
#include <Crypto/Crypto.h>
#include <cassert>
#include <set>

// Aggregate a vector of transactions into a multi-kernel transaction with cut_through.
TransactionPtr TransactionAggregator::Aggregate(const std::vector<TransactionPtr>& transactions)
{
	assert(!transactions.empty());

	if (transactions.size() == 1)
	{
		return transactions.front();
	}

	std::vector<TransactionInput> inputs;
	std::vector<TransactionOutput> outputs;
	std::vector<TransactionKernel> kernels;
	std::vector<BlindingFactor> kernelOffsets;

	// collect all the inputs, outputs and kernels from the txs
	for (const TransactionPtr& pTransaction : transactions)
	{
		for (const TransactionInput& input : pTransaction->GetInputs())
		{
			inputs.push_back(input);
		}

		for (const TransactionOutput& output : pTransaction->GetOutputs())
		{
			outputs.push_back(output);
		}

		for (const TransactionKernel& kernel : pTransaction->GetKernels())
		{
			kernels.push_back(kernel);
		}

		kernelOffsets.push_back(pTransaction->GetOffset());
	}

	// Perform cut-through
	TransactionUtil::PerformCutThrough(inputs, outputs);

	// Sort the kernels.
	std::sort(kernels.begin(), kernels.end(), SortKernelsByHash);
	std::sort(inputs.begin(), inputs.end(), SortInputsByHash);
	std::sort(outputs.begin(), outputs.end(), SortOutputsByHash);

	// Sum the kernel_offsets up to give us an aggregate offset for the transaction.
	BlindingFactor totalKernelOffset = Crypto::AddBlindingFactors(kernelOffsets, std::vector<BlindingFactor>());

	// Build a new aggregate tx from the following:
	//   * cut-through inputs
	//   * cut-through outputs
	//   * full set of tx kernels
	//   * sum of all kernel offsets
	return std::make_shared<Transaction>(std::move(totalKernelOffset), TransactionBody(std::move(inputs), std::move(outputs), std::move(kernels)));
}