#include "ValidTransactionFinder.h"
#include "TransactionAggregator.h"

#include <Core/Validation/TransactionValidator.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Common/Util/FunctionalUtil.h>
#include <PMMR/TxHashSetManager.h>
#include <Database/BlockDb.h>

ValidTransactionFinder::ValidTransactionFinder(TxHashSetManagerConstPtr pTxHashSetManager)
	: m_pTxHashSetManager(pTxHashSetManager)
{

}

std::vector<Transaction> ValidTransactionFinder::FindValidTransactions(
	std::shared_ptr<const IBlockDB> pBlockDB,
	const std::vector<Transaction>& transactions,
	const std::unique_ptr<Transaction>& pExtraTransaction) const
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
			if (IsValidTransaction(pBlockDB, *pAggregateTransaction))
			{
				validTransactions.push_back(transaction);
			}
		}
	}

	return validTransactions;
}

bool ValidTransactionFinder::IsValidTransaction(std::shared_ptr<const IBlockDB> pBlockDB, const Transaction& transaction) const
{
	if (!TransactionValidator().ValidateTransaction(transaction))
	{
		return false;
	}

	ITxHashSetConstPtr pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
	if (pTxHashSet == nullptr)
	{
		return false;
	}

	// Validate the tx against current chain state.
	// Check all inputs are in the current UTXO set.
	// Check all outputs are unique in current UTXO set.
	if (!pTxHashSet->IsValid(pBlockDB, transaction))
	{
		return false;
	}

	return true;
}