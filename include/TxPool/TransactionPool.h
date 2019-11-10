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

enum class EAddTransactionStatus
{
	ADDED,
	DUPLICATE,
	LOW_FEE,
	TX_INVALID,
	NOT_ADDED
};

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

	//
	// Validates and adds the transaction to the specified pool.
	// Returns:
	// * ADDED - If transaction was successfully added
	// * DUPLICATE - If transaction was already in the pool.
	// * LOW_FEE - If fee does not meet the minimum set in the config.
	// * TX_INVALID - If the transaction is not self-consistent. Source peer should be banned.
	// * NOT_ADDED - If the transaction is valid, but can't be added yet due to eg. lock-heights.
	//
	virtual EAddTransactionStatus AddTransaction(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const Transaction& transaction,
		const EPoolType poolType,
		const BlockHeader& lastConfirmedBlock
	) = 0;

	virtual std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const = 0;
	virtual std::unique_ptr<Transaction> FindTransactionByKernelHash(const Hash& kernelHash) const = 0;
	virtual void ReconcileBlock(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const FullBlock& block
	) = 0;

	// Dandelion
	virtual std::unique_ptr<Transaction> GetTransactionToStem(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet
	) = 0;
	virtual std::unique_ptr<Transaction> GetTransactionToFluff(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet
	) = 0;
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