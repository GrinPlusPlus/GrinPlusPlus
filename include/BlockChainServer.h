#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <BlockChainStatus.h>
#include <TxPool/PoolType.h>
#include "Core/ChainType.h"
#include "Core/BlockHeader.h"
#include "Core/FullBlock.h"
#include "Core/CompactBlock.h"
#include "Core/Transaction.h"
#include "BigInteger.h"

#include <vector>
#include <memory>

// Forward Declarations
class Config;
class IDatabase;

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
	virtual uint64_t GetHeight(const EChainType chainType) const = 0;
	virtual uint64_t GetTotalDifficulty(const EChainType chainType) const = 0;

	virtual EBlockChainStatus AddBlock(const FullBlock& block) = 0;
	virtual EBlockChainStatus AddCompactBlock(const CompactBlock& compactBlock) = 0;

	virtual EBlockChainStatus ProcessTransactionHashSet(const Hash& blockHash, const std::string& path) = 0;
	virtual EBlockChainStatus AddTransaction(const Transaction& transaction, const EPoolType poolType) = 0;

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
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByCommitment(const Hash& outputCommitment) const = 0;

	//
	// Returns the block headers matching the given hashes.
	//
	virtual std::vector<BlockHeader> GetBlockHeadersByHash(const std::vector<Hash>& blockHeaderHashes) const = 0;

	//
	// Returns the block matching the given hash.
	// This will be null if no matching block is found.
	//
	virtual std::unique_ptr<FullBlock> GetBlockByHash(const Hash& blockHash) const = 0;

	//
	// Returns the hashes of blocks(indexed by height) that are part of the candidate (header) chain, but whose bodies haven't been downloaded yet.
	//
	virtual std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const = 0;
};

namespace BlockChainAPI
{
	//
	// Creates a new instance of the BlockChain server.
	//
	BLOCK_CHAIN_API IBlockChainServer* StartBlockChainServer(const Config& config, IDatabase& database);

	//
	// Stops the BlockChain server and clears up its memory usage.
	//
	BLOCK_CHAIN_API void ShutdownBlockChainServer(IBlockChainServer* pBlockChainServer);
}