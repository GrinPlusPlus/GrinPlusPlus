#pragma once

#include "Chain.h"
#include "ChainStore.h"
#include "BlockStore.h"
#include "LockedChainState.h"
#include "OrphanPool/OrphanPool.h"

#include <Core/BlockHeader.h>
#include <Core/ChainType.h>
#include <HeaderMMR.h>
#include <Hash.h>
#include <shared_mutex>
#include <memory>

// Forward Declarations
class ITransactionPool;
class ITxHashSet;

class ChainState
{
public:
	ChainState(const Config& config, ChainStore& chainStore, BlockStore& blockStore, IHeaderMMR& headerMMR, ITransactionPool& transactionPool);
	~ChainState();

	void Initialize(const BlockHeader& genesisHeader);

	uint64_t GetHeight(const EChainType chainType);
	uint64_t GetTotalDifficulty(const EChainType chainType);

	std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const Hash& hash);
	std::unique_ptr<BlockHeader> GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType);
	std::unique_ptr<FullBlock> GetOrphanBlock(const Hash& hash) const;

	std::vector<std::pair<uint64_t, Hash>> GetBlocksNeeded(const uint64_t maxNumBlocks) const;

	LockedChainState GetLocked();
	void FlushAll();

private:
	std::unique_ptr<BlockHeader> GetHead_Locked(const EChainType chainType);
	const Hash& GetHeadHash_Locked(const EChainType chainType);

	mutable std::shared_mutex m_chainMutex;

	const Config& m_config;
	ChainStore& m_chainStore;
	BlockStore& m_blockStore;
	OrphanPool m_orphanPool;
	IHeaderMMR& m_headerMMR;
	ITransactionPool& m_transactionPool;
	std::shared_ptr<ITxHashSet> m_pTxHashSet;
};