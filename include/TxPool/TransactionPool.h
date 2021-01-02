#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <TxPool/DandelionStatus.h>
#include <TxPool/PoolType.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/ShortId.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/BlockHeader.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>
#include <Crypto/Hash.h>
#include <vector>
#include <set>

// Forward Declarations
class IBlockDB;

#define TX_POOL_API

enum class EAddTransactionStatus
{
	ADDED,
	DUPL_TX,
	LOW_FEE,
	TX_INVALID,
	NOT_ADDED
};

class ITransactionPool
{
public:
	using Ptr = std::shared_ptr<ITransactionPool>;
	using CPtr = std::shared_ptr<const ITransactionPool>;

	virtual ~ITransactionPool() = default;

	//
	// Retrieves txs from the mempool based on kernel short_ids from the compact block.
	// Note: does not validate that we return the full set of required txs.
	// The caller will need to validate that themselves.
	//
	virtual std::vector<TransactionPtr> GetTransactionsByShortId(
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
		TransactionPtr pTransaction,
		const EPoolType poolType,
		const BlockHeader& lastConfirmedBlock
	) = 0;

	virtual std::vector<TransactionPtr> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const = 0;
	virtual TransactionPtr FindTransactionByKernelHash(const Hash& kernelHash) const = 0;
	virtual void ReconcileBlock(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet,
		const FullBlock& block
	) = 0;

	// Dandelion
	virtual TransactionPtr GetTransactionToStem(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet
	) = 0;
	virtual TransactionPtr GetTransactionToFluff(
		std::shared_ptr<const IBlockDB> pBlockDB,
		ITxHashSetConstPtr pTxHashSet
	) = 0;
	virtual std::vector<TransactionPtr> GetExpiredTransactions() const = 0;
};

namespace TxPoolAPI
{
	//
	// Creates a new instance of the Transaction Pool.
	//
	TX_POOL_API ITransactionPool::Ptr CreateTransactionPool(
		const Config& config
	);
}