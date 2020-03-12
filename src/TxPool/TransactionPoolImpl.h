#pragma once

#include "Pool.h"

#include <TxPool/TransactionPool.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/ShortId.h>
#include <Crypto/Hash.h>
#include <shared_mutex>
#include <set>

class TransactionPool : public ITransactionPool
{
public:
	TransactionPool(const Config& config);
    virtual ~TransactionPool() = default;

	virtual std::vector<TransactionPtr> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const override final;
	virtual EAddTransactionStatus AddTransaction(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet, TransactionPtr pTransaction, const EPoolType poolType, const BlockHeader& lastConfirmedBlock) override final;
	virtual std::vector<TransactionPtr> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const override final;
	virtual TransactionPtr FindTransactionByKernelHash(const Hash& kernelHash) const override final;
	virtual void ReconcileBlock(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet, const FullBlock& block) override final;

	// Dandelion
	virtual TransactionPtr GetTransactionToStem(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet) override final;
	virtual TransactionPtr GetTransactionToFluff(std::shared_ptr<const IBlockDB> pBlockDB, ITxHashSetConstPtr pTxHashSet) override final;
	virtual std::vector<TransactionPtr> GetExpiredTransactions() const override final;

	// GrinJoin
	virtual void FluffJoinPool() override final;

private:
	const Config& m_config;
	mutable std::shared_mutex m_mutex;

	Pool m_memPool;
	Pool m_stemPool;
	Pool m_joinPool;
};
