#pragma once

#include "Pool.h"

#include <TxPool/TransactionPool.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/ShortId.h>
#include <Crypto/Hash.h>
#include <set>

class TransactionPool : public ITransactionPool
{
public:
	TransactionPool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB);

	virtual std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const override final;
	virtual bool AddTransaction(const Transaction& transaction, const EPoolType poolType, const BlockHeader& lastConfirmedBlock) override final; // TODO: Take in last block or BlockSums so we can verify kernel sums
	virtual std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const override final;
	virtual std::unique_ptr<Transaction> FindTransactionByKernelHash(const Hash& kernelHash) const override final;
	//virtual std::vector<Transaction> FindTransactionsByStatus(const EDandelionStatus status, const EPoolType poolType) const override final;
	virtual void ReconcileBlock(const FullBlock& block) override final;

	// Dandelion
	virtual std::unique_ptr<Transaction> GetTransactionToStem(const BlockHeader& lastConfirmedBlock) override final;
	virtual std::unique_ptr<Transaction> GetTransactionToFluff(const BlockHeader& lastConfirmedBlock) override final;
	virtual std::vector<Transaction> GetExpiredTransactions() const override final;

private:
	const Config& m_config;
	const TxHashSetManager& m_txHashSetManager;
	const IBlockDB& m_blockDB;

	Pool m_memPool;
	Pool m_stemPool;
};