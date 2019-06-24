#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <BlockChain/BlockChainStatus.h>
#include <TxPool/PoolType.h>
#include <P2P/SyncStatus.h>
#include <Core/Models/Display/BlockWithOutputs.h>
#include <BlockChain/ChainType.h>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/CompactBlock.h>
#include <Core/Models/Transaction.h>
#include <Crypto/BigInteger.h>

#include <vector>
#include <memory>

// Forward Declarations
class Config;
class IDatabase;
class TxHashSetManager;
class ITransactionPool;
class SyncStatus;

#ifdef MW_BLOCK_CHAIN
#define BLOCK_CHAIN_API EXPORT
#else
#define BLOCK_CHAIN_API IMPORT
#endif

//
// This interface acts as the single entry-point into the BlockChain shared library.
// This handles validation and in-memory storage of all block headers, transactions, and UTXOs.
//
class IBlockChainServer
{
public:
	virtual bool ResyncChain() = 0;

	virtual void UpdateSyncStatus(SyncStatus& syncStatus) const = 0;
	virtual uint64_t GetHeight(const EChainType chainType) const = 0;
	virtual uint64_t GetTotalDifficulty(const EChainType chainType) const = 0;

	virtual EBlockChainStatus AddBlock(const FullBlock& block) = 0;
	virtual EBlockChainStatus AddCompactBlock(const CompactBlock& compactBlock) = 0;

	virtual std::string SnapshotTxHashSet(const BlockHeader& blockHeader) = 0;
	virtual EBlockChainStatus ProcessTransactionHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus) = 0;
	virtual EBlockChainStatus AddTransaction(const Transaction& transaction, const EPoolType poolType) = 0;
	virtual std::unique_ptr<Transaction> GetTransactionByKernelHash(const Hash& kernelHash) const = 0;

	virtual EBlockChainStatus AddBlockHeader(const BlockHeader& blockHeader) = 0;

	//
	// Validates and adds the given block headers to the block chain.
	// All block headers that are successfully validated will be saved out to the database.
	// NOTE: For now, the block headers must be supplied in ascending order.
	//
	virtual EBlockChainStatus AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders) = 0;

	//
	// Returns the block header at the given height.
	// This will be null if no matching block header is found.
	//
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const = 0;

	//
	// Returns the block header matching the given hash.
	// This will be null if no matching block header is found.
	//
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const Hash& blockHeaderHash) const = 0;

	//
	// Returns the block header containing the output commitment.
	// This will be null if the output commitment is not found.
	//
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByCommitment(const Commitment& outputCommitment) const = 0;

	//
	// Returns the block header at the tip of the specified chain type.
	//
	virtual std::unique_ptr<BlockHeader> GetTipBlockHeader(const EChainType chainType) const = 0;

	//
	// Returns the block headers matching the given hashes.
	//
	virtual std::vector<BlockHeader> GetBlockHeadersByHash(const std::vector<Hash>& blockHeaderHashes) const = 0;

	//
	// Creates a compact block to represent the block with the given hash, if it exists.
	//
	virtual std::unique_ptr<CompactBlock> GetCompactBlockByHash(const Hash& hash) const = 0;

	//
	// Returns the block at the given height.
	// This will be null if no matching block is found.
	//
	virtual std::unique_ptr<FullBlock> GetBlockByHeight(const uint64_t height) const = 0;

	//
	// Returns the block matching the given hash.
	// This will be null if no matching block is found.
	//
	virtual std::unique_ptr<FullBlock> GetBlockByHash(const Hash& blockHash) const = 0;

	//
	// Returns the block containing the output commitment.
	// This will be null if the output commitment is not found or the block doesn't exist in the DB.
	//
	virtual std::unique_ptr<FullBlock> GetBlockByCommitment(const Commitment& outputCommitment) const = 0;


	virtual std::vector<BlockWithOutputs> GetOutputsByHeight(const uint64_t startHeight, const uint64_t maxHeight) const = 0;

	//
	// Returns the hashes of blocks(indexed by height) that are part of the candidate (header) chain, but whose bodies haven't been downloaded yet.
	//
	virtual std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const = 0;

	virtual bool ProcessNextOrphanBlock() = 0;
};

namespace BlockChainAPI
{
	//
	// Creates a new instance of the BlockChain server.
	//
	BLOCK_CHAIN_API IBlockChainServer* StartBlockChainServer(const Config& config, IDatabase& database, TxHashSetManager& txHashSetManager, ITransactionPool& transactionPool);

	//
	// Stops the BlockChain server and clears up its memory usage.
	//
	BLOCK_CHAIN_API void ShutdownBlockChainServer(IBlockChainServer* pBlockChainServer);
}