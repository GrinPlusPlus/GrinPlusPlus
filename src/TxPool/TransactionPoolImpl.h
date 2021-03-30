#pragma once

#include "Pool.h"

#include <TxPool/TransactionPool.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/ShortId.h>
#include <Crypto/Models/Hash.h>
#include <shared_mutex>
#include <set>

class TransactionPool : public ITransactionPool
{
public:
	TransactionPool(const Config& config)
		: m_config(config), m_memPool(), m_stemPool() { }
    virtual ~TransactionPool() = default;

	std::vector<TransactionPtr> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const final;
	EAddTransactionStatus AddTransaction(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet, TransactionPtr pTransaction, const EPoolType poolType, const BlockHeader& lastConfirmedBlock) final;
	std::vector<TransactionPtr> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const final;
	TransactionPtr FindTransactionByKernelHash(const Hash& kernelHash) const final;
	void ReconcileBlock(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet, const FullBlock& block) final;

	// Dandelion
	TransactionPtr GetTransactionToStem(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet) final;
	TransactionPtr GetTransactionToFluff(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet) final;
	std::vector<TransactionPtr> GetExpiredTransactions() const final;

private:
	const Config& m_config;
	mutable std::shared_mutex m_mutex;

	Pool m_memPool;
	Pool m_stemPool;
};
