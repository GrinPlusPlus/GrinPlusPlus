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
	//
	// Retrieves txs from the mempool based on kernel short_ids from the compact block.
	// Note: does not validate that we return the full set of required txs.
	// The caller will need to validate that themselves.
	//
	virtual std::vector<Transaction> GetTransactionsByShortId(
		const Hash& hash,
		const uint64_t nonce,
		const std::set<ShortId>& missingShortIds
	) const = 0;

	virtual bool AddTransaction(
		std::shared_ptr<const IBlockDB> pBlockDB,
		const Transaction& transaction,
		const EPoolType poolType,
		const BlockHeader& lastConfirmedBlock
	) = 0;

	virtual std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const = 0;
	virtual std::unique_ptr<Transaction> FindTransactionByKernelHash(const Hash& kernelHash) const = 0;
	virtual void ReconcileBlock(std::shared_ptr<const IBlockDB> pBlockDB, const FullBlock& block) = 0;

	// Dandelion
	virtual std::unique_ptr<Transaction> GetTransactionToStem(std::shared_ptr<const IBlockDB> pBlockDB) = 0;
	virtual std::unique_ptr<Transaction> GetTransactionToFluff(std::shared_ptr<const IBlockDB> pBlockDB) = 0;
	virtual std::vector<Transaction> GetExpiredTransactions() const = 0;
};

typedef std::shared_ptr<ITransactionPool> ITransactionPoolPtr;
typedef std::shared_ptr<const ITransactionPool> ITransactionPoolConstPtr;

namespace TxPoolAPI
{
	//
	// Creates a new instance of the Transaction Pool.
	//
	TX_POOL_API ITransactionPoolPtr CreateTransactionPool(
		const Config& config,
		TxHashSetManagerConstPtr pTxHashSetManager
	);
}