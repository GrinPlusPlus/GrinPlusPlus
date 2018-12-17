#pragma once

#include <Core/Transaction.h>
#include <Core/ShortId.h>
#include <Hash.h>
#include <mutex>
#include <set>
#include <map>

class TransactionPool
{
public:
	TransactionPool() = default;

	std::vector<Transaction> RetrieveTransactions(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const;
	void AddTransaction(const Transaction& transaction);
	void RemoveTransactions(const std::vector<Transaction>& transactions);

private:
	mutable std::mutex m_transactionsMutex;
	std::map<TransactionKernel, Transaction> m_transactionsByKernel;
};