#pragma once

#include "BlockStore.h"
#include "ChainState.h"
#include "ChainStore.h"
#include "TransactionPool.h"

#include <BlockChainServer.h>
#include <HeaderMMR.h>
#include <Database.h>
#include <stdint.h>
#include <map>
#include <mutex>

class BlockChainServer : public IBlockChainServer
{
public:
	BlockChainServer(const Config& config, IDatabase& database);
	~BlockChainServer();

	//
	// Initializes the blockchain by loading the previously downloaded and verified blocks from the database.
	// If this is the first time opening BitcoinDB (ie. no blockchain database exists), the blockchain is populated with only the genesis block.
	//
	void Initialize();
	void Shutdown();

	virtual uint64_t GetHeight(const EChainType chainType) const override final;
	virtual uint64_t GetTotalDifficulty(const EChainType chainType) const override final;

	virtual EBlockChainStatus AddBlock(const FullBlock& block) override final;
	virtual EBlockChainStatus AddCompactBlock(const CompactBlock& block) override final;

	virtual EBlockChainStatus AddBlockHeader(const BlockHeader& blockHeader) override final;
	virtual EBlockChainStatus AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders) override final;

	virtual EBlockChainStatus ProcessTransactionHashSet(const Hash& blockHash, const std::string& path) override final;
	virtual EBlockChainStatus AddTransaction(const Transaction& transaction) override final;

	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const override final;
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const CBigInteger<32>& hash) const override final;
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByCommitment(const Hash& outputCommitment) const override final;
	virtual std::vector<BlockHeader> GetBlockHeadersByHash(const std::vector<CBigInteger<32>>& hashes) const override final;

	virtual std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const override final;

private:
	bool m_initialized = { false };
	BlockStore* m_pBlockStore;
	ChainState* m_pChainState;
	ChainStore* m_pChainStore;
	IHeaderMMR* m_pHeaderMMR;
	TransactionPool* m_pTransactionPool;
	const Config& m_config;

	IDatabase& m_database;
};