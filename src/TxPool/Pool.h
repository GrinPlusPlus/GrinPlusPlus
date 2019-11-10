#pragma once

#include "TxPoolEntry.h"

#include <TxPool/DandelionStatus.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/ShortId.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <set>

class Pool
{
public:
	Pool() = default;
	~Pool() = default;

	void AddTransaction(const Transaction& transaction, const EDandelionStatus status);
	bool ContainsTransaction(const Transaction& transaction) const;
	void RemoveTransaction(const Transaction& transaction);
	void ReconcileBlock(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const FullBlock& block,
		const std::unique_ptr<Transaction>& pMemPoolAggTx
	);
	void ChangeStatus(const std::vector<Transaction>& transactions, const EDandelionStatus status);

	std::vector<Transaction> GetTransactionsByShortId(
		const Hash& hash,
		const uint64_t nonce,
		const std::set<ShortId>& missingShortIds
	) const;
	std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const;
	std::unique_ptr<Transaction> FindTransactionByKernelHash(const Hash& kernelHash) const;
	std::vector<Transaction> FindTransactionsByStatus(const EDandelionStatus status) const;
	std::vector<Transaction> GetExpiredTransactions(const uint16_t embargoSeconds) const;

	std::unique_ptr<Transaction> Aggregate() const;

private:
	bool ShouldEvict(const Transaction& transaction, const FullBlock& block) const;

	std::vector<TxPoolEntry> m_transactions;
};