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
		std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
		std::shared_ptr<ITransactionPool> pTransactionPool,
		std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR
	);

	void ResyncChain() final;

	void UpdateSyncStatus(SyncStatus& syncStatus) const final;
	uint64_t GetHeight(const EChainType chainType) const final;
	uint64_t GetTotalDifficulty(const EChainType chainType) const final;

	EBlockChainStatus AddBlock(const FullBlock& block) final;
	EBlockChainStatus AddCompactBlock(const CompactBlock& block) final;

	EBlockChainStatus AddBlockHeader(BlockHeaderPtr pBlockHeader) final;
	EBlockChainStatus AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders) final;

	fs::path SnapshotTxHashSet(BlockHeaderPtr pBlockHeader) final;
	EBlockChainStatus ProcessTransactionHashSet(const Hash& blockHash, const fs::path& path, SyncStatus& syncStatus) final;
	EBlockChainStatus AddTransaction(TransactionPtr pTransaction, const EPoolType poolType) final;
	TransactionPtr GetTransactionByKernelHash(const Hash& kernelHash) const final;

	BlockHeaderPtr GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const final;
	BlockHeaderPtr GetBlockHeaderByHash(const CBigInteger<32>& hash) const final;
	BlockHeaderPtr GetBlockHeaderByCommitment(const Commitment& outputCommitment) const final;
	BlockHeaderPtr GetTipBlockHeader(const EChainType chainType) const final;
	std::vector<BlockHeaderPtr> GetBlockHeadersByHash(const std::vector<CBigInteger<32>>& hashes) const final;

	std::unique_ptr<CompactBlock> GetCompactBlockByHash(const Hash& hash) const final;
	std::unique_ptr<FullBlock> GetBlockByCommitment(const Commitment& blockHash) const final;
	std::unique_ptr<FullBlock> GetBlockByHash(const Hash& blockHash) const final;
	std::unique_ptr<FullBlock> GetBlockByHeight(const uint64_t height) const final;
	bool HasBlock(const uint64_t height, const Hash& blockHash) const final;

	std::vector<BlockWithOutputs> GetOutputsByHeight(const uint64_t startHeight, const uint64_t maxHeight) const final;
	std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const final;

	bool ProcessNextOrphanBlock() final;

private:
	BlockChainServer(
		const Config& config,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
		std::shared_ptr<ITransactionPool> pTransactionPool,
		std::shared_ptr<Locked<ChainState>> pChainState,
		std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR
	);

	const Config& m_config;
	std::shared_ptr<Locked<IBlockDB>> m_pDatabase;
	std::shared_ptr<Locked<TxHashSetManager>> m_pTxHashSetManager;
	std::shared_ptr<ITransactionPool> m_pTransactionPool;
	std::shared_ptr<Locked<ChainState>> m_pChainState;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
};
