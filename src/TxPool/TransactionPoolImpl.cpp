#include "TransactionPoolImpl.h"
#include "TransactionAggregator.h"
#include "ValidTransactionFinder.h"

#include <Database/BlockDb.h>
#include <Consensus/BlockTime.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Infrastructure/Logger.h>
#include <Core/Util/FeeUtil.h>
#include <Core/Validation/TransactionValidator.h>

TransactionPool::TransactionPool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB)
	: m_config(config), 
	m_txHashSetManager(txHashSetManager), 
	m_blockDB(blockDB),
	m_memPool(config, txHashSetManager, blockDB),
	m_stemPool(config, txHashSetManager, blockDB)
{

}

std::vector<Transaction> TransactionPool::GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	return m_memPool.GetTransactionsByShortId(hash, nonce, missingShortIds);
}

bool TransactionPool::AddTransaction(const Transaction& transaction, const EPoolType poolType, const BlockHeader& lastConfirmedBlock)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	if (poolType == EPoolType::MEMPOOL && m_memPool.ContainsTransaction(transaction))
	{
		LOG_TRACE("Duplicate transaction " + transaction.GetHash().ToHex());
		return false;
	}

	// Verify fee meets minimum
	const uint64_t feeBase = 1000000; // TODO: Read from config.
	if (FeeUtil::CalculateMinimumFee(feeBase, transaction) > FeeUtil::CalculateActualFee(transaction))
	{
		LOG_WARNING("Fee too low for transaction " + transaction.GetHash().ToHex());
		return false;
	}
	
	// Verify lock time
	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		if (kernel.GetLockHeight() > (lastConfirmedBlock.GetHeight() + 1))
		{
			LOG_INFO("Invalid lock height: " + transaction.GetHash().ToHex());
			return false;
		}
	}

	if (!TransactionValidator().ValidateTransaction(transaction))
	{
		// TODO: Ban peer
		LOG_WARNING("Invalid transaction " + transaction.GetHash().ToHex());
		return false;
	}

	// Check all inputs are in current UTXO set & all outputs unique in current UTXO set
	const ITxHashSet* pTxHashSet = m_txHashSetManager.GetTxHashSet();
	if (pTxHashSet == nullptr || !pTxHashSet->IsValid(transaction))
	{
		LOG_WARNING("Transaction inputs/outputs not valid: " + transaction.GetHash().ToHex());
		return false;
	}

	if (poolType == EPoolType::MEMPOOL)
	{
		m_memPool.AddTransaction(transaction, EDandelionStatus::FLUFFED);
		m_stemPool.RemoveTransaction(transaction);
	}
	else if (poolType == EPoolType::STEMPOOL)
	{
		const uint8_t random = (uint8_t)RandomNumberGenerator::GenerateRandom(0, 100);
		if (random <= m_config.GetDandelionConfig().GetStemProbability())
		{
			LOG_INFO("Stemming transaction " + transaction.GetHash().ToHex());
			m_stemPool.AddTransaction(transaction, EDandelionStatus::TO_STEM);
		}
		else
		{
			LOG_INFO("Fluffing transaction " + transaction.GetHash().ToHex());
			m_stemPool.AddTransaction(transaction, EDandelionStatus::TO_FLUFF);
		}
	}

	return true;
}

std::vector<Transaction> TransactionPool::FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	return m_memPool.FindTransactionsByKernel(kernels);
}

std::unique_ptr<Transaction> TransactionPool::FindTransactionByKernelHash(const Hash& kernelHash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::unique_ptr<Transaction> pTransaction = m_memPool.FindTransactionByKernelHash(kernelHash);
	if (pTransaction == nullptr)
	{
		pTransaction = m_stemPool.FindTransactionByKernelHash(kernelHash);
	}

	return pTransaction;
}

void TransactionPool::ReconcileBlock(const FullBlock& block)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	// First reconcile the txpool.
	m_memPool.ReconcileBlock(block, std::unique_ptr<Transaction>(nullptr));

	// Now reconcile our stempool, accounting for the updated txpool txs.
	const std::unique_ptr<Transaction> pMemPoolAggTx = m_memPool.Aggregate();
	m_stemPool.ReconcileBlock(block, pMemPoolAggTx);
}

std::unique_ptr<Transaction> TransactionPool::GetTransactionToStem()
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	const std::vector<Transaction> transactionsToStem = m_stemPool.FindTransactionsByStatus(EDandelionStatus::TO_STEM);
	if (transactionsToStem.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const std::unique_ptr<Transaction> pMemPoolAggTx = m_memPool.Aggregate();

	std::vector<Transaction> validTransactionsToStem = ValidTransactionFinder(m_txHashSetManager).FindValidTransactions(transactionsToStem, pMemPoolAggTx);
	if (validTransactionsToStem.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	std::unique_ptr<Transaction> pTransactionToStem = TransactionAggregator::Aggregate(validTransactionsToStem);
	if (pTransactionToStem != nullptr)
	{
		m_stemPool.ChangeStatus(validTransactionsToStem, EDandelionStatus::STEMMED);
	}

	return pTransactionToStem;
}

std::unique_ptr<Transaction> TransactionPool::GetTransactionToFluff()
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	const std::vector<Transaction> transactionsToFluff = m_stemPool.FindTransactionsByStatus(EDandelionStatus::TO_FLUFF);
	if (transactionsToFluff.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const std::unique_ptr<Transaction> pMemPoolAggTx = m_memPool.Aggregate();

	std::vector<Transaction> validTransactionsToFluff = ValidTransactionFinder(m_txHashSetManager).FindValidTransactions(transactionsToFluff, pMemPoolAggTx);
	if (validTransactionsToFluff.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	std::unique_ptr<Transaction> pTransactionToFluff = TransactionAggregator::Aggregate(validTransactionsToFluff);
	if (pTransactionToFluff != nullptr)
	{
		m_memPool.AddTransaction(*pTransactionToFluff, EDandelionStatus::FLUFFED);
		for (const Transaction& transaction : validTransactionsToFluff)
		{
			m_stemPool.RemoveTransaction(transaction);
		}
	}

	return pTransactionToFluff;
}

std::vector<Transaction> TransactionPool::GetExpiredTransactions() const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	const uint16_t embargoSeconds = m_config.GetDandelionConfig().GetEmbargoSeconds() + (uint16_t)RandomNumberGenerator::GenerateRandom(0, 30);
	return m_stemPool.GetExpiredTransactions(embargoSeconds);
}

namespace TxPoolAPI
{
	TX_POOL_API ITransactionPool* CreateTransactionPool(
		const Config& config,
		const TxHashSetManager& txHashSetManager,
		const IBlockDB& blockDB)
	{
		return new TransactionPool(config, txHashSetManager, blockDB);
	}

	TX_POOL_API void DestroyTransactionPool(ITransactionPool* pTransactionPool)
	{
		delete ((TransactionPool*)pTransactionPool);
	}
}