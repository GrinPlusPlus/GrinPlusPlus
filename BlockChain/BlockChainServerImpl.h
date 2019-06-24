#pragma once

#include "BlockStore.h"
#include "ChainState.h"
#include "ChainStore.h"

#include <TxPool/TransactionPool.h>
#include <BlockChain/BlockChainServer.h>
#include <PMMR/HeaderMMR.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <P2P/SyncStatus.h>
#include <stdint.h>
#include <mutex>

class BlockChainServer : public IBlockChainServer
{
public:
	BlockChainServer(const Config& config, IDatabase& database, TxHashSetManager& txHashSetManager, ITransactionPool& transactionPool);
	~BlockChainServer();

	//
	// Initializes the blockchain by loading the previously downloaded and verified blocks from the database.
	// If this is the first time opening BitcoinDB (ie. no blockchain database exists), the blockchain is populated with only the genesis block.
	//
	void Initialize();
	void Shutdown();
	virtual bool ResyncChain() override final;

	virtual void UpdateSyncStatus(SyncStatus& syncStatus) const override final;
	virtual uint64_t GetHeight(const EChainType chainType) const override final;
	virtual uint64_t GetTotalDifficulty(const EChainType chainType) const override final;

	virtual EBlockChainStatus AddBlock(const FullBlock& block) override final;
	virtual EBlockChainStatus AddCompactBlock(const CompactBlock& block) override final;

	virtual EBlockChainStatus AddBlockHeader(const BlockHeader& blockHeader) override final;
	virtual EBlockChainStatus AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders) override final;

	virtual std::string SnapshotTxHashSet(const BlockHeader& blockHeader) override final;
	virtual EBlockChainStatus ProcessTransactionHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus) override final;
	virtual EBlockChainStatus AddTransaction(const Transaction& transaction, const EPoolType poolType) override final;
	virtual std::unique_ptr<Transaction> GetTransactionByKernelHash(const Hash& kernelHash) const override final;

	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const override final;
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const CBigInteger<32>& hash) const override final;
	virtual std::unique_ptr<BlockHeader> GetBlockHeaderByCommitment(const Commitment& outputCommitment) const override final;
	virtual std::unique_ptr<BlockHeader> GetTipBlockHeader(const EChainType chainType) const override final;
	virtual std::vector<BlockHeader> GetBlockHeadersByHash(const std::vector<CBigInteger<32>>& hashes) const override final;

	virtual std::unique_ptr<CompactBlock> GetCompactBlockByHash(const Hash& hash) const override final;
	virtual std::unique_ptr<FullBlock> GetBlockByCommitment(const Commitment& blockHash) const override final;
	virtual std::unique_ptr<FullBlock> GetBlockByHash(const Hash& blockHash) const override final;
	virtual std::unique_ptr<FullBlock> GetBlockByHeight(const uint64_t height) const override final;

	virtual std::vector<BlockWithOutputs> GetOutputsByHeight(const uint64_t startHeight, const uint64_t maxHeight) const override final;
	virtual std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const override final;

	virtual bool ProcessNextOrphanBlock() override final;

private:
	bool m_initialized = { false };
	BlockStore* m_pBlockStore;
	ChainState* m_pChainState;
	ChainStore* m_pChainStore;
	IHeaderMMR* m_pHeaderMMR;
	ITransactionPool& m_transactionPool;

	const Config& m_config;
	IDatabase& m_database;
	TxHashSetManager& m_txHashSetManager;
};