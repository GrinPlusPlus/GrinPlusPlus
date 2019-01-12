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
	std::unique_ptr<Commitment> AddCommitments(const Transaction& transaction, const BlockSums& blockSums) const;
	std::unique_ptr<Commitment> AddKernelOffsets(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const;

	const TxHashSetManager& m_txHashSetManager;
	const IBlockDB& m_blockDB;
};