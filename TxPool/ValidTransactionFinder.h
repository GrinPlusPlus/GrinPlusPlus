#pragma once

#include <Core/Models/Transaction.h>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/BlockSums.h>

// Forward Declarations
class TxHashSetManager;
class IBlockDB;

class ValidTransactionFinder
{
public:
	ValidTransactionFinder(const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB);

	std::vector<Transaction> FindValidTransactions(const std::vector<Transaction>& transactions, const std::unique_ptr<Transaction>& pExtraTransaction, const BlockHeader& header) const;

private:
	bool IsValidTransaction(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const;
	bool ValidateKernelSums(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const;

	const TxHashSetManager& m_txHashSetManager;
	const IBlockDB& m_blockDB;
};