#pragma once

#include "TxPoolEntry.h"

#include <TxPool/DandelionStatus.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/ShortId.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>
#include <Database/BlockDb.h>
#include <Crypto/Hash.h>
#include <map>
#include <set>
#include <shared_mutex>

class Pool
{
public:
	Pool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB);

	bool AddTransaction(const Transaction& transaction, const EDandelionStatus status);
	bool ContainsTransaction(const Transaction& transaction) const;
	void RemoveTransaction(const Transaction& transaction);
	void ReconcileBlock(const FullBlock& block, const std::unique_ptr<Transaction>& pMemPoolAggTx);

	std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const;
	std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const;
	std::unique_ptr<Transaction> FindTransactionByKernelHash(const Hash& kernelHash) const;
	std::vector<Transaction> FindTransactionsByStatus(const EDandelionStatus status) const;
	std::vector<Transaction> GetExpiredTransactions(const uint16_t embargoSeconds) const;

	std::unique_ptr<Transaction> Aggregate() const;

private:
	bool ShouldEvict_Locked(const Transaction& transaction, const FullBlock& block) const;

	const Config& m_config;
	const TxHashSetManager& m_txHashSetManager;
	const IBlockDB& m_blockDB;

	mutable std::shared_mutex m_transactionsMutex; // TODO: Lock belongs in TransactionPoolImpl, instead.
	std::vector<TxPoolEntry> m_transactions;
};