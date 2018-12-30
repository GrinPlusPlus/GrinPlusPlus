#include "Pool.h"
#include "TransactionValidator.h"

#include <VectorUtil.h>
#include <algorithm>

// Query the tx pool for all known txs based on kernel short_ids from the provided compact_block.
// Note: does not validate that we return the full set of required txs. The caller will need to validate that themselves.
std::vector<Transaction> Pool::GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);

	std::vector<Transaction> transactionsFound;
	for (const Transaction& transaction : m_transactions)
	{
		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			const ShortId shortId = ShortId::Create(kernel.GetHash(), hash, nonce);
			if (missingShortIds.find(shortId) != missingShortIds.cend())
			{
				transactionsFound.push_back(transaction);

				if (transactionsFound.size() == missingShortIds.size())
				{
					return transactionsFound;
				}
			}
		}
	}

	return transactionsFound;
}

bool Pool::AddTransaction(const Transaction& transaction)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	if (TransactionValidator().ValidateTransaction(transaction))
	{
		m_transactions.push_back(transaction);
		return true;
	}

	return false;
}

std::vector<Transaction> Pool::FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);

	std::set<Transaction> transactionSet;
	for (const Transaction& transaction : m_transactions)
	{
		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			if (kernels.count(kernel) > 0)
			{
				transactionSet.insert(transaction);
			}
		}
	}

	std::vector<Transaction> transactions;
	std::copy(transactionSet.begin(), transactionSet.end(), std::back_inserter(transactions));

	return transactions;
}

void Pool::RemoveTransactions(const std::vector<Transaction>& transactions)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	auto iter = m_transactions.begin();
	while (iter != m_transactions.end())
	{
		if (std::find(transactions.begin(), transactions.end(), *iter) != transactions.end())
		{
			m_transactions.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

// Quick reconciliation step - we can evict any txs in the pool where
// inputs or kernels intersect with the block.
void Pool::ReconcileBlock(const FullBlock& block)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	// Filter txs in the pool based on the latest block.
	// Reject any txs where we see a matching tx kernel in the block.
	// Also reject any txs where we see a conflicting tx,
	// where an input is spent in a different tx.
	auto iter = m_transactions.begin();
	while (iter != m_transactions.end())
	{
		if (ShouldEvict_Locked(*iter, block))
		{
			m_transactions.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

bool Pool::ShouldEvict_Locked(const Transaction& transaction, const FullBlock& block) const
{
	const std::vector<TransactionInput>& blockInputs = block.GetTransactionBody().GetInputs();
	for (const TransactionInput& input : transaction.GetBody().GetInputs())
	{
		if (std::find(blockInputs.begin(), blockInputs.end(), input) != blockInputs.end())
		{
			return true;
		}
	}

	const std::vector<TransactionKernel>& blockKernels = block.GetTransactionBody().GetKernels();
	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		if (std::find(blockKernels.begin(), blockKernels.end(), kernel) != blockKernels.end())
		{
			return true;
		}
	}

	return false;
}