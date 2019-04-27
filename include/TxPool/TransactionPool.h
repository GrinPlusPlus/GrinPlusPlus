#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <TxPool/DandelionStatus.h>
#include <TxPool/PoolType.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/ShortId.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/BlockSums.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <vector>
#include <set>

// Forward Declarations
class IBlockDB;

#ifdef MW_TX_POOL
#define TX_POOL_API EXPORT
#else
#define TX_POOL_API IMPORT
#endif

class ITransactionPool
{
public:
	virtual std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const = 0;
	virtual bool AddTransaction(const Transaction& transaction, const EPoolType poolType, const BlockHeader& lastConfirmedBlock) = 0;
	virtual std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const = 0;
	virtual std::unique_ptr<Transaction> FindTransactionByKernelHash(const Hash& kernelHash) const = 0;
	//virtual std::vector<Transaction> FindTransactionsByStatus(const EDandelionStatus status, const EPoolType poolType) const = 0;
	virtual void ReconcileBlock(const FullBlock& block) = 0;

	// Dandelion
	virtual std::unique_ptr<Transaction> GetTransactionToStem(const BlockHeader& lastConfirmedHeader) = 0;
	virtual std::unique_ptr<Transaction> GetTransactionToFluff(const BlockHeader& lastConfirmedHeader) = 0;
	virtual std::vector<Transaction> GetExpiredTransactions() const = 0;

	// TODO: Prepare Mineable Transactions
};

namespace TxPoolAPI
{
	//
	// Creates a new instance of the Transaction Pool.
	//
	TX_POOL_API ITransactionPool* CreateTransactionPool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB);

	//
	// Destroys the instance of the Transaction Pool
	//
	TX_POOL_API void DestroyTransactionPool(ITransactionPool* pTransactionPool);
}