#include "TransactionPoolImpl.h"
#include "ValidTransactionFinder.h"

#include <Core/Util/TransactionUtil.h>
#include <Database/BlockDb.h>
#include <Consensus/BlockTime.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Common/Logger.h>
#include <Core/Util/FeeUtil.h>
#include <Core/Validation/TransactionValidator.h>

TransactionPool::TransactionPool(const Config& config)
	: m_config(config), 
	m_memPool(),
	m_stemPool()
{

}

std::vector<TransactionPtr> TransactionPool::GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	return m_memPool.GetTransactionsByShortId(hash, nonce, missingShortIds);
}

EAddTransactionStatus TransactionPool::AddTransaction(
	std::shared_ptr<const IBlockDB> pBlockDB,
	ITxHashSetConstPtr pTxHashSet,
	TransactionPtr pTransaction,
	const EPoolType poolType,
	const BlockHeader& lastConfirmedBlock)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	if (poolType == EPoolType::MEMPOOL && m_memPool.ContainsTransaction(*pTransaction))
	{
		LOG_TRACE_F("Duplicate transaction ({})", *pTransaction);
		return EAddTransactionStatus::DUPL_TX;
	}

	// Verify fee meets minimum
	const uint64_t feeBase = 1000000; // TODO: Read from config.
	if (FeeUtil::CalculateMinimumFee(feeBase, *pTransaction) > FeeUtil::CalculateActualFee(*pTransaction))
	{
		LOG_WARNING_F("Fee too low for transaction ({})", *pTransaction);
		return EAddTransactionStatus::LOW_FEE;
	}
	
	// Verify lock time
	for (const TransactionKernel& kernel : pTransaction->GetKernels())
	{
		if (kernel.GetLockHeight() > (lastConfirmedBlock.GetHeight() + 1))
		{
			LOG_INFO_F("Invalid lock height ({})", *pTransaction);
			return EAddTransactionStatus::NOT_ADDED;
		}
	}

	try
	{
		TransactionValidator().Validate(*pTransaction);
	}
	catch (std::exception& e)
	{
		LOG_WARNING_F("Invalid transaction ({}). Error: ({})", *pTransaction, e.what());
		return EAddTransactionStatus::TX_INVALID;
	}

	// Check all inputs are in current UTXO set & all outputs unique in current UTXO set
	if (pTxHashSet == nullptr || !pTxHashSet->IsValid(pBlockDB, *pTransaction))
	{
		LOG_WARNING_F("Transaction inputs/outputs not valid ({})", *pTransaction);
		return EAddTransactionStatus::NOT_ADDED;
	}

	if (poolType == EPoolType::MEMPOOL)
	{
		m_memPool.AddTransaction(pTransaction, EDandelionStatus::FLUFFED);
		m_stemPool.RemoveTransaction(*pTransaction);
	}
	else if (poolType == EPoolType::STEMPOOL)
	{
		const uint8_t random = (uint8_t)RandomNumberGenerator::GenerateRandom(0, 100);
		if (random <= m_config.GetNodeConfig().GetDandelion().GetStemProbability())
		{
			LOG_INFO_F("Stemming transaction ({})", *pTransaction);
			m_stemPool.AddTransaction(pTransaction, EDandelionStatus::TO_STEM);
		}
		else
		{
			LOG_INFO_F("Fluffing transaction ({})", *pTransaction);
			m_stemPool.AddTransaction(pTransaction, EDandelionStatus::TO_FLUFF);
		}
	}
	else if (poolType == EPoolType::JOINPOOL)
	{
		m_joinPool.AddTransaction(pTransaction, EDandelionStatus::TO_FLUFF);
	}

	return EAddTransactionStatus::ADDED;
}

std::vector<TransactionPtr> TransactionPool::FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	return m_memPool.FindTransactionsByKernel(kernels);
}

TransactionPtr TransactionPool::FindTransactionByKernelHash(const Hash& kernelHash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	TransactionPtr pTransaction = m_memPool.FindTransactionByKernelHash(kernelHash);
	if (pTransaction == nullptr)
	{
		pTransaction = m_stemPool.FindTransactionByKernelHash(kernelHash);
	}

	return pTransaction;
}

void TransactionPool::ReconcileBlock(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet, const FullBlock& block)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	// First reconcile the txpool.
	m_memPool.ReconcileBlock(pBlockDB, pTxHashSet, block, nullptr);

	// Now reconcile our stempool, accounting for the updated txpool txs.
	auto pMemPoolAggTx = m_memPool.Aggregate();
	m_stemPool.ReconcileBlock(pBlockDB, pTxHashSet, block, pMemPoolAggTx);
	m_joinPool.ReconcileBlock(pBlockDB, pTxHashSet, block, pMemPoolAggTx);
}

TransactionPtr TransactionPool::GetTransactionToStem(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	const std::vector<TransactionPtr> transactionsToStem = m_stemPool.FindTransactionsByStatus(EDandelionStatus::TO_STEM);
	if (transactionsToStem.empty())
	{
		return nullptr;
	}

	auto pMemPoolAggTx = m_memPool.Aggregate();

	std::vector<TransactionPtr> validTransactionsToStem = ValidTransactionFinder::FindValidTransactions(
		pBlockDB,
		pTxHashSet,
		transactionsToStem,
		pMemPoolAggTx
	);
	if (validTransactionsToStem.empty())
	{
		return nullptr;
	}

	TransactionPtr pTransactionToStem = TransactionUtil::Aggregate(validTransactionsToStem);

	m_stemPool.ChangeStatus(validTransactionsToStem, EDandelionStatus::STEMMED);

	return pTransactionToStem;
}

TransactionPtr TransactionPool::GetTransactionToFluff(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	std::vector<TransactionPtr> transactionsToFluff = m_stemPool.FindTransactionsByStatus(EDandelionStatus::TO_FLUFF);
	if (transactionsToFluff.empty())
	{
		return nullptr;
	}

	auto pMemPoolAggTx = m_memPool.Aggregate();

	std::vector<TransactionPtr> validTransactionsToFluff = ValidTransactionFinder::FindValidTransactions(
		pBlockDB,
		pTxHashSet,
		transactionsToFluff,
		pMemPoolAggTx
	);
	if (validTransactionsToFluff.empty())
	{
		return nullptr;
	}

	TransactionPtr pTransactionToFluff = TransactionUtil::Aggregate(validTransactionsToFluff);

	m_memPool.AddTransaction(pTransactionToFluff, EDandelionStatus::FLUFFED);
	for (auto& pTransaction : validTransactionsToFluff)
	{
		m_stemPool.RemoveTransaction(*pTransaction);
	}

	return pTransactionToFluff;
}

std::vector<TransactionPtr> TransactionPool::GetExpiredTransactions() const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	const uint16_t embargoSeconds = m_config.GetNodeConfig().GetDandelion().GetEmbargoSeconds() + (uint16_t)RandomNumberGenerator::GenerateRandom(0, 30);
	return m_stemPool.GetExpiredTransactions(embargoSeconds);
}

void TransactionPool::FluffJoinPool()
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	TransactionPtr pAggregatedTx = m_joinPool.Aggregate();
	if (pAggregatedTx != nullptr)
	{
		LOG_INFO_F("Fluffing transaction with {} kernels", pAggregatedTx->GetKernels().size());
		m_stemPool.AddTransaction(pAggregatedTx, EDandelionStatus::TO_FLUFF);
	}

	m_joinPool.Clear();
}

namespace TxPoolAPI
{
	TX_POOL_API std::shared_ptr<ITransactionPool> CreateTransactionPool(const Config& config)
	{
		return std::shared_ptr<TransactionPool>(new TransactionPool(config));
	}
}