#pragma once

#include <Core/Models/Transaction.h>
#include <shared_mutex>
#include <vector>

class GrinJoinPool
{
public:
	std::vector<TransactionPtr> GetTransactions()
	{
	    std::unique_lock<std::shared_mutex> writeLock(m_mutex);

		std::vector<TransactionPtr> transactions = m_transactions;
	    m_transactions.clear();

		return transactions;
	}

private:
	std::shared_mutex m_mutex;
	std::vector<TransactionPtr> m_transactions;
};