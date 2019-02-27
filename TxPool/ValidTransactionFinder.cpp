#include "ValidTransactionFinder.h"
#include "TransactionAggregator.h"

#include <Core/Validation/TransactionValidator.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Common/Util/FunctionalUtil.h>
#include <PMMR/TxHashSetManager.h>
#include <Database/BlockDb.h>

ValidTransactionFinder::ValidTransactionFinder(const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB)
	: m_txHashSetManager(txHashSetManager), m_blockDB(blockDB)
{

}

std::vector<Transaction> ValidTransactionFinder::FindValidTransactions(const std::vector<Transaction>& transactions, const std::unique_ptr<Transaction>& pExtraTransaction, const BlockHeader& header) const
{
	std::unique_ptr<BlockSums> pBlockSums = m_blockDB.GetBlockSums(header.GetHash());
	if (pBlockSums == nullptr)
	{
		return std::vector<Transaction>();
	}

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
			if (IsValidTransaction(*pAggregateTransaction, header, *pBlockSums))
			{
				validTransactions.push_back(transaction);
			}
		}
	}

	return validTransactions;
}

bool ValidTransactionFinder::IsValidTransaction(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const
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

	if (!ValidateKernelSums(transaction, header, blockSums))
	{
		return false;
	}

	return true;
}

// Verify the sum of the kernel excesses equals the sum of the outputs, taking into account both the kernel_offset and overage.
bool ValidTransactionFinder::ValidateKernelSums(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const
{
	// Calculate overage
	uint64_t overage = 0;
	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		overage += kernel.GetFee();
	}

	// Calculate the offset
	const std::unique_ptr<BlindingFactor> pOffset = Crypto::AddBlindingFactors(std::vector<BlindingFactor>({ header.GetTotalKernelOffset(), transaction.GetOffset() }), std::vector<BlindingFactor>());
	if (pOffset == nullptr)
	{
		return false;
	}

	std::unique_ptr<BlockSums> pCalculatedBlockSums = KernelSumValidator::ValidateKernelSums(transaction.GetBody(), (int64_t)overage, *pOffset, std::make_optional<BlockSums>(blockSums));
	if (pCalculatedBlockSums == nullptr)
	{
		return false;
	}

	return true;
}