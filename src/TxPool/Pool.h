#pragma once

#include "TxPoolEntry.h"

#include <TxPool/DandelionStatus.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/ShortId.h>
#include <Core/Config.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <set>

class Pool
{
public:
	Pool() = default;
	~Pool() = default;

	void AddTransaction(TransactionPtr pTransaction, const EDandelionStatus status);
	bool ContainsTransaction(const Transaction& transaction) const;
	void RemoveTransaction(const Transaction& transaction);
	void ReconcileBlock(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const FullBlock& block,
		TransactionPtr pMemPoolAggTx
	);
	void ChangeStatus(const std::vector<TransactionPtr>& transactions, const EDandelionStatus status);

	std::vector<TransactionPtr> GetTransactionsByShortId(
		const Hash& hash,
		const uint64_t nonce,
		const std::set<ShortId>& missingShortIds
	) const;
	std::vector<TransactionPtr> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const;
	TransactionPtr FindTransactionByKernelHash(const Hash& kernelHash) const;
	std::vector<TransactionPtr> FindTransactionsByStatus(const EDandelionStatus status) const;
	std::vector<TransactionPtr> GetExpiredTransactions(const uint16_t embargoSeconds) const;

	TransactionPtr Aggregate() const;
	void Clear() { m_transactions.clear(); }

private:
	bool ShouldEvict(const Transaction& transaction, const FullBlock& block) const;

	std::vector<TxPoolEntry> m_transactions;
};