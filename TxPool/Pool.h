#pragma once

#include "TxPoolEntry.h"

#include <TxPool/DandelionStatus.h>
#include <Core/Transaction.h>
#include <Core/TransactionKernel.h>
#include <Core/FullBlock.h>
#include <Core/ShortId.h>
#include <Hash.h>
#include <map>
#include <set>
#include <shared_mutex>

class Pool
{
public:
	bool AddTransaction(const Transaction& transaction, const EDandelionStatus status);
	void RemoveTransactions(const std::vector<Transaction>& transactions);
	void ReconcileBlock(const FullBlock& block);

	std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const;
	std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const;
	std::vector<Transaction> FindTransactionsByStatus(const EDandelionStatus status) const;
	std::vector<Transaction> GetExpiredTransactions(const uint16_t embargoSeconds) const;

	std::unique_ptr<Transaction> Aggregate() const;

private:
	bool ShouldEvict_Locked(const Transaction& transaction, const FullBlock& block) const;

	mutable std::shared_mutex m_transactionsMutex;
	std::vector<TxPoolEntry> m_transactions;
};