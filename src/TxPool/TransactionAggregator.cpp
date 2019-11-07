#include "TransactionAggregator.h"

#include <Core/Util/TransactionUtil.h>
#include <Crypto/Crypto.h>
#include <set>

// Aggregate a vector of transactions into a multi-kernel transaction with cut_through.
Transaction TransactionAggregator::Aggregate(const std::vector<Transaction>& transactions)
{
	if (transactions.empty())
	{
		return Transaction(BlindingFactor(ZERO_HASH), TransactionBody());
	}

	if (transactions.size() == 1)
	{
		return transactions.front();
	}

	std::vector<TransactionInput> inputs;
	std::vector<TransactionOutput> outputs;
	std::vector<TransactionKernel> kernels;
	std::vector<BlindingFactor> kernelOffsets;

	// collect all the inputs, outputs and kernels from the txs
	for (const Transaction& transaction : transactions)
	{
		for (const TransactionInput& input : transaction.GetBody().GetInputs())
		{
			inputs.push_back(input);
		}

		for (const TransactionOutput& output : transaction.GetBody().GetOutputs())
		{
			outputs.push_back(output);
		}

		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			kernels.push_back(kernel);
		}

		kernelOffsets.push_back(transaction.GetOffset());
	}

	// Sort inputs and outputs during cut_through.
	TransactionUtil::PerformCutThrough(inputs, outputs);

	// Sort the kernels.
	std::sort(kernels.begin(), kernels.end(), SortKernelsByHash);

	// Sum the kernel_offsets up to give us an aggregate offset for the transaction.
	BlindingFactor totalKernelOffset = Crypto::AddBlindingFactors(kernelOffsets, std::vector<BlindingFactor>());

	// Build a new aggregate tx from the following:
	//   * cut-through inputs
	//   * cut-through outputs
	//   * full set of tx kernels
	//   * sum of all kernel offsets
	return Transaction(std::move(totalKernelOffset), TransactionBody(std::move(inputs), std::move(outputs), std::move(kernels)));
}