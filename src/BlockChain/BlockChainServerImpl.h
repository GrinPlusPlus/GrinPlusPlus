#pragma once

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
	//
	// Initializes the blockchain by loading the previously downloaded and verified blocks from the database.
	// If this is the first time opening BitcoinDB (ie. no blockchain database exists), the blockchain is populated with only the genesis block.
	//
	static std::shared_ptr<BlockChainServer> Create(
		const Config& config,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		std::shared_ptr<TxHashSetManager> pTxHashSetManager,
		std::shared_ptr<ITransactionPool> pTransactionPool
	);

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
	virtual EBlockChainStatus AddTransaction(TransactionPtr pTransaction, const EPoolType poolType) override final;
	virtual TransactionPtr GetTransactionByKernelHash(const Hash& kernelHash) const override final;

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
	BlockChainServer(
		const Config& config,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		std::shared_ptr<TxHashSetManager> pTxHashSetManager,
		std::shared_ptr<ITransactionPool> pTransactionPool,
		std::shared_ptr<Locked<ChainState>> pChainState,
		std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR
	);

	const Config& m_config;
	std::shared_ptr<Locked<IBlockDB>> m_pDatabase;
	TxHashSetManagerPtr m_pTxHashSetManager;
	std::shared_ptr<ITransactionPool> m_pTransactionPool;
	std::shared_ptr<Locked<ChainState>> m_pChainState;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
};
