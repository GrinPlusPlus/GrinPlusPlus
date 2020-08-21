#pragma once

#include <Core/Models/Transaction.h>
#include <Core/Models/BlockHeader.h>
#include <PMMR/TxHashSet.h>

// Forward Declarations
class IBlockDB;

class ValidTransactionFinder
{
public:
	static std::vector<TransactionPtr> FindValidTransactions(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const std::vector<TransactionPtr>& transactions,
		TransactionPtr pExtraTransaction
	);

private:
	static bool IsValidTransaction(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		TransactionPtr pTransaction
	);
};