#pragma once

#include "TxPoolEntry.h"

#include <TxPool/DandelionStatus.h>
#include <Core/Transaction.h>
#include <Core/TransactionKernel.h>
#include <Core/FullBlock.h>
#include <Core/ShortId.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>
#include <Database/BlockDb.h>
#include <Hash.h>
#include <lru/cache.hpp>
#include <map>
#include <set>
#include <shared_mutex>

class Pool
{
public:
	Pool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB, LRU::Cache<Commitment, Commitment>& bulletproofsCache);

	bool AddTransaction(const Transaction& transaction, const EDandelionStatus status);
	void RemoveTransactions(const std::vector<Transaction>& transactions);
	void ReconcileBlock(const FullBlock& block, const std::unique_ptr<Transaction>& pMemPoolAggTx);

	std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const;
	std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const;
	std::vector<Transaction> FindTransactionsByStatus(const EDandelionStatus status) const;
	std::vector<Transaction> GetExpiredTransactions(const uint16_t embargoSeconds) const;

	std::unique_ptr<Transaction> Aggregate() const;

private:
	bool ShouldEvict_Locked(const Transaction& transaction, const FullBlock& block) const;

	const Config& m_config;
	const TxHashSetManager& m_txHashSetManager;
	const IBlockDB& m_blockDB;
	LRU::Cache<Commitment, Commitment>& m_bulletproofsCache;

	mutable std::shared_mutex m_transactionsMutex;
	std::vector<TxPoolEntry> m_transactions;
};