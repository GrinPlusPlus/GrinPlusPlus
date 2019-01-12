#pragma once

#include "../ChainState.h"

#include <Config/Config.h>
#include <Core/FullBlock.h>
#include <BlockChainStatus.h>

class BlockProcessor
{
public:
	BlockProcessor(const Config& config, ChainState& chainState);

	EBlockChainStatus ProcessBlock(const FullBlock& block);

private:
	EBlockChainStatus ProcessBlockInternal(const FullBlock& block);
	EBlockChainStatus ProcessNextBlock(const FullBlock& block, LockedChainState& lockedState);
	EBlockChainStatus ProcessOrphanBlock(const FullBlock& block, LockedChainState& lockedState);

	bool ShouldOrphan(const FullBlock& block, LockedChainState& lockedState);

	const Config& m_config;
	ChainState& m_chainState;
};