#pragma once

#include <Core/Models/Transaction.h>
#include <Core/Models/BlockHeader.h>

// Forward Declarations
class TxHashSetManager;

class ValidTransactionFinder
{
public:
	ValidTransactionFinder(const TxHashSetManager& txHashSetManager);

	std::vector<Transaction> FindValidTransactions(const std::vector<Transaction>& transactions, const std::unique_ptr<Transaction>& pExtraTransaction) const;

private:
	bool IsValidTransaction(const Transaction& transaction) const;

	const TxHashSetManager& m_txHashSetManager;
};