#pragma once

#include <TxPool/TransactionPool.h>
#include <Core/Transaction.h>
#include <Core/ShortId.h>
#include <Hash.h>
#include <shared_mutex>
#include <set>
#include <map>

class TransactionPool : public ITransactionPool
{
public:
	virtual std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const override final;
	virtual bool AddTransaction(const Transaction& transaction, const EPoolType poolType) override final;
	virtual std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const override final;
	virtual void RemoveTransactions(const std::vector<Transaction>& transactions, const EPoolType poolType) override final;
	virtual void ReconcileBlock(const FullBlock& block) override final;

	virtual bool ValidateTransaction(const Transaction& transaction) const override final;
	virtual bool ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward) const override final;

private:
	mutable std::shared_mutex m_transactionsMutex;
	std::map<Hash, Transaction> m_transactionsByKernelHash;
	std::map<Hash, Transaction> m_transactionsByTxHash;
};