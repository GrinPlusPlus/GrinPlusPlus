#pragma once

#include "Chain.h"
#include "ChainStore.h"
#include "BlockStore.h"
#include "LockedChainState.h"
#include "OrphanPool/OrphanPool.h"

#include <P2P/SyncStatus.h>
#include <Core/Models/BlockHeader.h>
#include <BlockChain/ChainType.h>
#include <Core/Models/Display/BlockWithOutputs.h>
#include <PMMR/HeaderMMR.h>
#include <Crypto/Hash.h>
#include <shared_mutex>

// Forward Declarations
class ITransactionPool;
class TxHashSetManager;

class ChainState
{
public:
	ChainState(const Config& config, ChainStore& chainStore, BlockStore& blockStore, IHeaderMMR& headerMMR, ITransactionPool& transactionPool, TxHashSetManager& txHashSetManager);
	~ChainState();

	void Initialize(const BlockHeader& genesisHeader);

	void UpdateSyncStatus(SyncStatus& syncStatus) const;
	uint64_t GetHeight(const EChainType chainType) const;
	uint64_t GetTotalDifficulty(const EChainType chainType) const;

	std::unique_ptr<BlockHeader> GetTipBlockHeader(const EChainType chainType) const;
	std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const Hash& hash) const;
	std::unique_ptr<BlockHeader> GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const;
	std::unique_ptr<BlockHeader> GetBlockHeaderByCommitment(const Commitment& outputCommitment) const;

	std::unique_ptr<FullBlock> GetBlockByHash(const Hash& hash) const;
	std::unique_ptr<FullBlock> GetBlockByHeight(const uint64_t height) const;
	std::unique_ptr<FullBlock> GetOrphanBlock(const uint64_t height, const Hash& hash) const;

	std::unique_ptr<BlockWithOutputs> GetBlockWithOutputs(const uint64_t height) const;

	std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const;

	LockedChainState GetLocked();
	void FlushAll();

private:
	std::unique_ptr<BlockHeader> GetHead_Locked(const EChainType chainType) const;
	const Hash& GetHeadHash_Locked(const EChainType chainType) const;

	mutable std::shared_mutex m_chainMutex;

	const Config& m_config;
	ChainStore& m_chainStore;
	BlockStore& m_blockStore;
	OrphanPool m_orphanPool;
	IHeaderMMR& m_headerMMR;
	ITransactionPool& m_transactionPool;
	TxHashSetManager& m_txHashSetManager;
};