#pragma once

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
	bool AddTransaction(const Transaction& transaction);
	void RemoveTransactions(const std::vector<Transaction>& transactions);
	void ReconcileBlock(const FullBlock& block);

	std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const;
	std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const;

private:
	bool ShouldEvict_Locked(const Transaction& transaction, const FullBlock& block) const;

	mutable std::shared_mutex m_transactionsMutex;
	std::vector<Transaction> m_transactions;
};