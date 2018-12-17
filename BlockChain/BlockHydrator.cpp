#include "BlockHydrator.h"

BlockHydrator::BlockHydrator(const ChainState& chainState, const TransactionPool& transactionPool)
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
		const Hash& hash = compactBlock.GetBlockHeader().Hash();
		const uint64_t nonce = compactBlock.GetNonce();
		const std::vector<Transaction> transactions = m_transactionPool.RetrieveTransactions(hash, nonce, std::set<ShortId>(shortIds.cbegin(), shortIds.cend()));

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
		// TODO: Make these more efficient by pre-allocating size. Also, create "AddAll" in VectorUtil
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

	// TODO: Need to sort allInputs, allOutputs, and allKernels.
	// TODO: Need to perform cut-through.

	// Create a Transaction Body.
	TransactionBody transactionBody(std::move(allInputs), std::move(allOutputs), std::move(allKernels));

	// Finally return the full block.
	// Note: we have not actually validated the block here, caller must validate the block.
	BlockHeader header = compactBlock.GetBlockHeader();
	return std::make_unique<FullBlock>(FullBlock(std::move(header), std::move(transactionBody)));
}