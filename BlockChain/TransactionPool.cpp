#include "TransactionPool.h"

// Query the tx pool for all known txs based on kernel short_ids from the provided compact_block.
// Note: does not validate that we return the full set of required txs. The caller will need to validate that themselves.
std::vector<Transaction> TransactionPool::RetrieveTransactions(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	std::lock_guard<std::mutex> lockGuard(m_transactionsMutex);

	std::vector<Transaction> transactionsFound;

	for (const auto& transactionByKernel : m_transactionsByKernel)
	{
		const TransactionKernel& kernel = transactionByKernel.first;
		const ShortId shortId = ShortId::Create(kernel.Hash(), hash, nonce);
		if (missingShortIds.find(shortId) != missingShortIds.cend())
		{
			transactionsFound.push_back(transactionByKernel.second);

			if (transactionsFound.size() == missingShortIds.size())
			{
				return transactionsFound;
			}
		}
	}

	return transactionsFound;
}

void TransactionPool::AddTransaction(const Transaction& transaction)
{
	std::lock_guard<std::mutex> lockGuard(m_transactionsMutex);

	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		m_transactionsByKernel.insert({ kernel, transaction });
	}
}

void TransactionPool::RemoveTransactions(const std::vector<Transaction>& transactions)
{
	std::lock_guard<std::mutex> lockGuard(m_transactionsMutex);

	for (const Transaction& transaction : transactions)
	{
		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			m_transactionsByKernel.erase(kernel);
		}
	}
}