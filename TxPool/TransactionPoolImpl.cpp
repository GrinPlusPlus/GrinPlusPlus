#include "TransactionPoolImpl.h"
#include "TransactionValidator.h"
#include "TransactionBodyValidator.h"

// Query the tx pool for all known txs based on kernel short_ids from the provided compact_block.
// Note: does not validate that we return the full set of required txs. The caller will need to validate that themselves.
std::vector<Transaction> TransactionPool::GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	return m_memPool.GetTransactionsByShortId(hash, nonce, missingShortIds);
}

bool TransactionPool::AddTransaction(const Transaction& transaction, const EPoolType poolType)
{
	if (poolType == EPoolType::MEMPOOL)
	{
		return m_memPool.AddTransaction(transaction);
	}
	else if (poolType == EPoolType::STEMPOOL)
	{
		return m_stemPool.AddTransaction(transaction);
	}

	return false;
}

std::vector<Transaction> TransactionPool::FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const
{
	return m_memPool.FindTransactionsByKernel(kernels);
}

void TransactionPool::RemoveTransactions(const std::vector<Transaction>& transactions, const EPoolType poolType)
{
	if (poolType == EPoolType::MEMPOOL)
	{
		m_memPool.RemoveTransactions(transactions);
	}
	else if (poolType == EPoolType::STEMPOOL)
	{
		m_stemPool.RemoveTransactions(transactions);
	}
}

void TransactionPool::ReconcileBlock(const FullBlock& block)
{
	// TODO: Finish implementing
	// First reconcile the txpool.
	m_memPool.ReconcileBlock(block);
	//self.txpool.reconcile(None, &block.header) ? ;

	// Now reconcile our stempool, accounting for the updated txpool txs.
	m_stemPool.ReconcileBlock(block);
	//let txpool_tx = self.txpool.aggregate_transaction() ? ;
	//self.stempool.reconcile(txpool_tx, &block.header) ? ;
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