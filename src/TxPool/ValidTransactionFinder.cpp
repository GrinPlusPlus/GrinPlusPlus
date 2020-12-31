#include "ValidTransactionFinder.h"

#include <Core/Util/TransactionUtil.h>
#include <Core/Validation/TransactionValidator.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Common/Util/FunctionalUtil.h>
#include <PMMR/TxHashSetManager.h>
#include <Database/BlockDb.h>

std::vector<TransactionPtr> ValidTransactionFinder::FindValidTransactions(
	std::shared_ptr<const IBlockDB> pBlockDB,
	ITxHashSetConstPtr pTxHashSet,
	const std::vector<TransactionPtr>& transactions,
	TransactionPtr pExtraTransaction)
{
	std::vector<TransactionPtr> validTransactions;
	for (TransactionPtr pTransaction : transactions)
	{
		std::vector<TransactionPtr> candidateTransactions = validTransactions;
		if (pExtraTransaction != nullptr)
		{
			candidateTransactions.push_back(pExtraTransaction);
		}

		candidateTransactions.push_back(pTransaction);

		// Build a single aggregate tx from candidate txs.
		TransactionPtr pAggregatedTransaction = TransactionUtil::Aggregate(candidateTransactions);

		// We know the tx is valid if the entire aggregate tx is valid.
		if (IsValidTransaction(pBlockDB, pTxHashSet, pAggregatedTransaction))
		{
			validTransactions.push_back(pTransaction);
		}
	}

	return validTransactions;
}

bool ValidTransactionFinder::IsValidTransaction(
	std::shared_ptr<const IBlockDB> pBlockDB,
	ITxHashSetConstPtr pTxHashSet,
	TransactionPtr pTransaction)
{
	try
	{
		const uint64_t next_block_height = pTxHashSet->GetFlushedBlockHeader()->GetHeight() + 1;
		TransactionValidator().Validate(*pTransaction, next_block_height);

		// Validate the tx against current chain state.
		// Check all inputs are in the current UTXO set.
		// Check all outputs are unique in current UTXO set.
		if (!pTxHashSet->IsValid(pBlockDB, *pTransaction))
		{
			return false;
		}
	}
	catch (std::exception&)
	{
		return false;
	}

	return true;
}