#include "BlockHydrator.h"

#include <Common/FunctionalUtil.h>

BlockHydrator::BlockHydrator(const ChainState& chainState, const ITransactionPool& transactionPool)
	: m_chainState(chainState), m_transactionPool(transactionPool)
{

}

std::unique_ptr<FullBlock> BlockHydrator::Hydrate(const CompactBlock& compactBlock) const
{
	const std::vector<ShortId>& shortIds = compactBlock.GetShortIds();
	if (shortIds.empty())
	{
		return Hydrate(compactBlock, std::vector<Transaction>());
	}
	else
	{
		const Hash& hash = compactBlock.GetBlockHeader().GetHash();
		const uint64_t nonce = compactBlock.GetNonce();
		const std::vector<Transaction> transactions = m_transactionPool.GetTransactionsByShortId(hash, nonce, std::set<ShortId>(shortIds.cbegin(), shortIds.cend()));

		if (transactions.size() == shortIds.size())
		{
			return Hydrate(compactBlock, transactions);
		}
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockHydrator::Hydrate(const CompactBlock& compactBlock, const std::vector<Transaction>& transactions) const
{
	std::set<TransactionInput> inputsSet;
	std::set<TransactionOutput> outputsSet;
	std::set<TransactionKernel> kernelsSet;

	// collect all the inputs, outputs and kernels from the txs
	for (const Transaction& transaction : transactions)
	{
		for (const TransactionInput& input : transaction.GetBody().GetInputs())
		{
			inputsSet.insert(input);
		}

		for (const TransactionOutput& output : transaction.GetBody().GetOutputs())
		{
			outputsSet.insert(output);
		}

		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			kernelsSet.insert(kernel);
		}
	}

	// include the coinbase output(s) and kernel(s) from the compact_block
	for (const TransactionOutput& output : compactBlock.GetOutputs())
	{
		outputsSet.insert(output);
	}

	for (const TransactionKernel& kernel : compactBlock.GetKernels())
	{
		kernelsSet.insert(kernel);
	}

	// convert the sets to vecs
	std::vector<TransactionInput> allInputs{ inputsSet.cbegin(), inputsSet.cend() };
	std::vector<TransactionOutput> allOutputs{ outputsSet.cbegin(), outputsSet.cend() };
	std::vector<TransactionKernel> allKernels{ kernelsSet.cbegin(), kernelsSet.cend() };

	// Perform cut-through.
	PerformCutThrough(allInputs, allOutputs);

	// Sort allInputs, allOutputs, and allKernels.
	std::sort(allInputs.begin(), allInputs.end());
	std::sort(allOutputs.begin(), allOutputs.end());
	std::sort(allKernels.begin(), allKernels.end());

	// Create a Transaction Body.
	TransactionBody transactionBody(std::move(allInputs), std::move(allOutputs), std::move(allKernels));

	// Finally return the full block.
	// Note: we have not actually validated the block here, caller must validate the block.
	BlockHeader header = compactBlock.GetBlockHeader();
	return std::make_unique<FullBlock>(FullBlock(std::move(header), std::move(transactionBody)));
}

void BlockHydrator::PerformCutThrough(std::vector<TransactionInput>& inputs, std::vector<TransactionOutput>& outputs) const
{
	std::set<Commitment> inputCommitments;
	for (const TransactionInput& input : inputs)
	{
		inputCommitments.insert(input.GetCommitment());
	}

	std::set<Commitment> outputCommitments;
	for (const TransactionOutput& output : outputs)
	{
		outputCommitments.insert(output.GetCommitment());
	}

	auto filterInputs = [outputCommitments](TransactionInput& input) -> bool { return outputCommitments.count(input.GetCommitment()) > 0; };
	FunctionalUtil::filter(inputs, filterInputs);

	auto filterOutputs = [inputCommitments](TransactionOutput& output) -> bool { return inputCommitments.count(output.GetCommitment()) > 0; };
	FunctionalUtil::filter(outputs, filterOutputs);
}