#pragma once

#include "../ChainState.h"

#include <Core/FullBlock.h>
#include <BlockChainStatus.h>

class BlockProcessor
{
public:
	BlockProcessor(ChainState& chainState);

	EBlockChainStatus ProcessBlock(const FullBlock& block);

private:
	EBlockChainStatus ProcessBlockInternal(const FullBlock& block);
	EBlockChainStatus ProcessNextBlock(const FullBlock& block, LockedChainState& lockedState);
	EBlockChainStatus ProcessOrphanBlock(const FullBlock& block, LockedChainState& lockedState);

	bool ShouldOrphan(const FullBlock& block, LockedChainState& lockedState);

	ChainState& m_chainState;
};