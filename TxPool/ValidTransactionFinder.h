#pragma once

#include "BulletProofsCache.h"

#include <Core/Models/Transaction.h>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/BlockSums.h>
#include <PMMR/TxHashSetManager.h>
#include <Database/BlockDb.h>

class ValidTransactionFinder
{
public:
	ValidTransactionFinder(const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB, BulletProofsCache& bulletproofsCache);

	std::vector<Transaction> FindValidTransactions(const std::vector<Transaction>& transactions, const std::unique_ptr<Transaction>& pExtraTransaction, const BlockHeader& header) const;

private:
	bool IsValidTransaction(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const;
	bool ValidateKernelSums(const Transaction& transaction, const BlockHeader& header, const BlockSums& blockSums) const;

	const TxHashSetManager& m_txHashSetManager;
	const IBlockDB& m_blockDB;
	BulletProofsCache& m_bulletproofsCache;
};