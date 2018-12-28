#include "TransactionPoolImpl.h"
#include "TransactionValidator.h"
#include "TransactionBodyValidator.h"

// Query the tx pool for all known txs based on kernel short_ids from the provided compact_block.
// Note: does not validate that we return the full set of required txs. The caller will need to validate that themselves.
std::vector<Transaction> TransactionPool::GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);

	std::vector<Transaction> transactionsFound;

	for (const auto& transactionByKernelHash : m_transactionsByKernelHash)
	{
		const Hash& kernelHash = transactionByKernelHash.first;
		const ShortId shortId = ShortId::Create(kernelHash, hash, nonce);
		if (missingShortIds.find(shortId) != missingShortIds.cend())
		{
			transactionsFound.push_back(transactionByKernelHash.second);

			if (transactionsFound.size() == missingShortIds.size())
			{
				return transactionsFound;
			}
		}
	}

	return transactionsFound;
}

bool TransactionPool::AddTransaction(const Transaction& transaction, const EPoolType poolType)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	if (ValidateTransaction(transaction))
	{
		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			m_transactionsByKernelHash.insert({ kernel.GetHash(), transaction });
		}

		m_transactionsByTxHash.insert({ transaction.GetHash(), transaction });

		return true;
	}

	return false;
}

std::vector<Transaction> TransactionPool::FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const
{
	// TODO: Implement
	return std::vector<Transaction>();
}

void TransactionPool::RemoveTransactions(const std::vector<Transaction>& transactions, const EPoolType poolType)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	for (const Transaction& transaction : transactions)
	{
		for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
		{
			m_transactionsByKernelHash.erase(kernel.GetHash());
		}

		m_transactionsByTxHash.erase(transaction.GetHash());
	}
}

void TransactionPool::ReconcileBlock(const FullBlock& block)
{
	// TODO: Implement
}

bool TransactionPool::ValidateTransaction(const Transaction& transaction) const
{
	return TransactionValidator().ValidateTransaction(transaction);
}

bool TransactionPool::ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward) const
{
	return TransactionBodyValidator().ValidateTransactionBody(transactionBody, withReward);
}

namespace TxPoolAPI
{
	TX_POOL_API ITransactionPool* CreateTransactionPool(const Config& config)
	{
		return new TransactionPool();
	}

	TX_POOL_API void DestroyTransactionPool(ITransactionPool* pTransactionPool)
	{
		delete ((TransactionPool*)pTransactionPool);
	}
}