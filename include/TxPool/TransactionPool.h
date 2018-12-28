#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <TxPool/PoolType.h>
#include <Core/Transaction.h>
#include <Core/ShortId.h>
#include <Core/FullBlock.h>
#include <Config/Config.h>
#include <Hash.h>
#include <vector>
#include <set>

#ifdef MW_TX_POOL
#define TX_POOL_API EXPORT
#else
#define TX_POOL_API IMPORT
#endif

class ITransactionPool
{
public:
	virtual std::vector<Transaction> RetrieveTransactions(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const = 0;
	virtual bool AddTransaction(const Transaction& transaction, const EPoolType poolType) = 0;
	virtual std::vector<Transaction> FindMatchingTransactions(const std::set<TransactionKernel>& kernels) const = 0;
	virtual void RemoveTransactions(const std::vector<Transaction>& transactions, const EPoolType poolType) = 0;
	virtual void ReconcileBlock(const FullBlock& block) = 0;

	virtual bool ValidateTransaction(const Transaction& transaction) const = 0;
	virtual bool ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward) const = 0;

	// TODO: Prepare Mineable Transactions
};

namespace TxPoolAPI
{
	//
	// Creates a new instance of the Transaction Pool.
	//
	TX_POOL_API ITransactionPool* CreateTransactionPool(const Config& config);

	//
	// Destroys the instance of the Transaction Pool
	//
	TX_POOL_API void DestroyTransactionPool(ITransactionPool* pTransactionPool);
}