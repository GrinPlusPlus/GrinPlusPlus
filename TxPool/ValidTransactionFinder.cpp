#include "ValidTransactionFinder.h"
#include "TransactionAggregator.h"
#include "TransactionValidator.h"

#include <Common/FunctionalUtil.h>

ValidTransactionFinder::ValidTransactionFinder(const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB)
	: m_txHashSetManager(txHashSetManager), m_blockDB(blockDB)
{

}

std::vector<Transaction> ValidTransactionFinder::FindValidTransactions(const std::vector<Transaction>& transactions, const std::unique_ptr<Transaction>& pExtraTransaction, const BlockHeader& header) const
{
	std::vector<Transaction> validTransactions;

	for (const Transaction& transaction : transactions)
	{
		std::vector<Transaction> candidateTransactions = validTransactions;
		if (pExtraTransaction != nullptr)
		{
			candidateTransactions.push_back(*pExtraTransaction);
		}

		candidateTransactions.push_back(transaction);

		// Build a single aggregate tx from candidate txs.
		std::unique_ptr<Transaction> pAggregateTransaction = TransactionAggregator::Aggregate(candidateTransactions);
		if (pAggregateTransaction != nullptr)
		{
			// We know the tx is valid if the entire aggregate tx is valid.
			if (IsValidTransaction(*pAggregateTransaction, header))
			{
				validTransactions.push_back(transaction);
			}
		}
	}

	return validTransactions;
}

bool ValidTransactionFinder::IsValidTransaction(const Transaction& transaction, const BlockHeader& header) const
{
	if (!TransactionValidator().ValidateTransaction(transaction))
	{
		return false;
	}

	const ITxHashSet* pTxHashSet = m_txHashSetManager.GetTxHashSet();
	if (pTxHashSet == nullptr)
	{
		return false;
	}

	// Validate the tx against current chain state.
	// Check all inputs are in the current UTXO set.
	// Check all outputs are unique in current UTXO set.
	if (!pTxHashSet->IsValid(transaction))
	{
		return false;
	}

	std::unique_ptr<BlockSums> pBlockSums = m_blockDB.GetBlockSums(header.GetHash());
	if (pBlockSums == nullptr)
	{
		return false;
	}

	std::unique_ptr<Commitment> pUTXOSum = AddCommitments(transaction, *pBlockSums);
	if (pUTXOSum == nullptr)
	{
		return false;
	}

	std::unique_ptr<Commitment> pKernelSumPlusOffset = AddKernelOffsets(transaction, header, *pBlockSums);
	if (pKernelSumPlusOffset == nullptr)
	{
		return false;
	}

	// Verify the kernel sums for the block_sums with the new tx applied, accounting for overage and offset.
	if (*pUTXOSum != *pKernelSumPlusOffset)
	{
		return false;
	}

	return true;
}

std::unique_ptr<Commitment> ValidTransactionFinder::AddCommitments(const Transaction& transaction, const BlockSums& blockSums) const
{
	// Calculate overage
	uint64_t overage = 0;
	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		overage += kernel.GetFee();
	}

	std::unique_ptr<Commitment> pOverageCommitment = Crypto::CommitTransparent(overage);

	// Gather the commitments
	auto getInputCommitments = [](TransactionInput& input) -> Commitment { return input.GetCommitment(); };
	std::vector<Commitment> inputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetInputs(), getInputCommitments);

	auto getOutputCommitments = [](TransactionOutput& output) -> Commitment { return output.GetCommitment(); };
	std::vector<Commitment> outputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetOutputs(), getOutputCommitments);

	outputCommitments.push_back(blockSums.GetOutputSum());

	// add the overage as output commitment if positive,
	// or as an input commitment if negative
	if (overage != 0)
	{
		outputCommitments.push_back(*pOverageCommitment);
	}

	return Crypto::AddCommitments(outputCommitments, inputCommitments);
}

std::unique_ptr<Commitment> ValidTransactionFinder::AddKernelOffsets(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const
{
	// Calculate the offset
	const std::unique_ptr<BlindingFactor> pOffset = Crypto::AddBlindingFactors(std::vector<BlindingFactor>({ header.GetTotalKernelOffset(), transaction.GetOffset() }), std::vector<BlindingFactor>());
	if (pOffset == nullptr)
	{
		return std::unique_ptr<Commitment>(nullptr);
	}

	// Gather the kernel excess commitments
	auto getKernelCommitments = [](TransactionKernel& kernel) -> Commitment { return kernel.GetExcessCommitment(); };
	std::vector<Commitment> kernelCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetKernels(), getKernelCommitments);

	kernelCommitments.push_back(blockSums.GetKernelSum());

	if (*pOffset != BlindingFactor(CBigInteger<32>::ValueOf(0)))
	{
		// Commit to zero.
		std::unique_ptr<Commitment> pOffsetCommitment = Crypto::CommitBlinded((uint64_t)0, *pOffset);
		kernelCommitments.push_back(*pOffsetCommitment);
	}

	// sum the commitments
	return Crypto::AddCommitments(kernelCommitments, std::vector<Commitment>({}));
}