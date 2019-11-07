#pragma once

#include <Core/Models/Transaction.h>
#include <Core/Models/BlockHeader.h>
#include <PMMR/TxHashSetManager.h>

class ValidTransactionFinder
{
public:
	ValidTransactionFinder();

	std::vector<Transaction> FindValidTransactions(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const std::vector<Transaction>& transactions,
		const std::unique_ptr<Transaction>& pExtraTransaction
	) const;

private:
	bool IsValidTransaction(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const Transaction& transaction
	) const;

	TxHashSetManagerConstPtr m_pTxHashSetManager;
};