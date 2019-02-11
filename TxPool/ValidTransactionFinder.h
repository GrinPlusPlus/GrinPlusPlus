#pragma once

#include <Core/Transaction.h>
#include <Core/BlockHeader.h>
#include <Core/BlockSums.h>
#include <PMMR/TxHashSetManager.h>
#include <Database/BlockDb.h>

class ValidTransactionFinder
{
public:
	ValidTransactionFinder(const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB);

	std::vector<Transaction> FindValidTransactions(const std::vector<Transaction>& transactions, const std::unique_ptr<Transaction>& pExtraTransaction, const BlockHeader& header) const;

private:
	bool IsValidTransaction(const Transaction& transaction, const BlockHeader& header) const;
	bool ValidateKernelSums(const Transaction& transaction, const BlockHeader& header) const;

	const TxHashSetManager& m_txHashSetManager;
	const IBlockDB& m_blockDB;
};