#pragma once

#include "Chain.h"
#include "ChainStore.h"
#include "BlockStore.h"
#include "LockedChainState.h"
#include "OrphanPool.h"

#include <Core/BlockHeader.h>
#include <Core/ChainType.h>
#include <HeaderMMR.h>
#include <Hash.h>
#include <shared_mutex>
#include <map>
#include <set>
#include <atomic>

// TODO: Rename BlockChainManager?
class ChainState
{
public:
	ChainState(ChainStore& chainStore, BlockStore& blockStore, IHeaderMMR& headerMMR);
	~ChainState();

	void Initialize(const BlockHeader& genesisHeader);

	uint64_t GetHeight(const EChainType chainType);
	uint64_t GetTotalDifficulty(const EChainType chainType);

	std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const Hash& hash);
	std::unique_ptr<BlockHeader> GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType);

	void BlockValidated(const Hash& hash);
	bool HasBlockBeenValidated(const Hash& hash) const;

	LockedChainState GetLocked();

private:
	std::unique_ptr<BlockHeader> GetHead_Locked(const EChainType chainType);
	const Hash& GetHeadHash_Locked(const EChainType chainType);

	mutable std::shared_mutex m_headersMutex;

	ChainStore& m_chainStore;
	BlockStore& m_blockStore;
	OrphanPool m_orphanPool;

	IHeaderMMR& m_headerMMR;

	// TODO: Figure out best approach to storing orphans. Probably could just store them by hash, and iterate over them daily (weekly?) and get rid of anything beyond horizon.
	std::set<Hash> m_validatedBlocks;
};