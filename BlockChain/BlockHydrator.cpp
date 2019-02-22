#include "BlockHydrator.h"

#include <Core/Util/TransactionUtil.h>
#include <Core/Validation/CutThroughVerifier.h>
#include <unordered_set>

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
	std::unordered_set<Hash> inputsSet;
	std::unordered_set<Hash> outputsSet;
	std::unordered_set<Hash> kernelsSet;

	std::vector<TransactionInput> allInputs;
	std::vector<TransactionOutput> allOutputs;
	std::vector<TransactionKernel> allKernels;

	// collect all the inputs, outputs and kernels from the txs
	for (const Transaction& transaction : transactions)
	{
		for (const TransactionInput& input : transaction.GetBody().GetInputs())
		{
			if (inputsSet.count(input.GetHash()) == 0)
			{
				inputsSet.insert(input.GetHash());
				allInputs.push_back(input);
			}
		}

		for (const TransactionOutput& output : transaction.GetBody().GetOutputs())
		{
			if (outputsSet.count(output.GetHash()) == 0)
			{
				outputsSet.insert(output.GetHash());
				allOutputs.push_back(output);
			}
		}

		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			if (kernelsSet.count(kernel.GetHash()) == 0)
			{
				kernelsSet.insert(kernel.GetHash());
				allKernels.push_back(kernel);
			}
		}
	}

	// include the coinbase output(s) and kernel(s) from the compact_block
	for (const TransactionOutput& output : compactBlock.GetOutputs())
	{
		if (outputsSet.count(output.GetHash()) == 0)
		{
			outputsSet.insert(output.GetHash());
			allOutputs.push_back(output);
		}
	}

	for (const TransactionKernel& kernel : compactBlock.GetKernels())
	{
		if (kernelsSet.count(kernel.GetHash()) == 0)
		{
			kernelsSet.insert(kernel.GetHash());
			allKernels.push_back(kernel);
		}
	}

	// Perform cut-through.
	TransactionUtil::PerformCutThrough(allInputs, allOutputs);

	// Sort allInputs, allOutputs, and allKernels.
	std::sort(allInputs.begin(), allInputs.end(), SortInputsByHash);
	std::sort(allOutputs.begin(), allOutputs.end(), SortOutputsByHash);
	std::sort(allKernels.begin(), allKernels.end(), SortKernelsByHash);

	// Create a Transaction Body.
	TransactionBody transactionBody(std::move(allInputs), std::move(allOutputs), std::move(allKernels));

	// Finally return the full block.
	// Note: we have not actually validated the block here, caller must validate the block.
	BlockHeader header = compactBlock.GetBlockHeader();
	return std::make_unique<FullBlock>(FullBlock(std::move(header), std::move(transactionBody)));
}