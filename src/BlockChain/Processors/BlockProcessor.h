#pragma once

#include "../ChainState.h"

#include <Config/Config.h>
#include <Core/Models/FullBlock.h>
#include <BlockChain/BlockChainStatus.h>

enum class EBlockStatus
{
	NEXT_BLOCK,
	REORG,
	ORPHAN
};

class BlockProcessor
{
public:
	BlockProcessor(const Config& config, std::shared_ptr<Locked<ChainState>> pChainState);

	EBlockChainStatus ProcessBlock(const FullBlock& block);

private:
	EBlockChainStatus ProcessBlockInternal(const FullBlock& block);
	void HandleReorg(const FullBlock& block, Writer<ChainState> pLockedState);
	void ValidateAndAddBlock(const FullBlock& block, Writer<ChainState> pLockedState);

	EBlockStatus DetermineBlockStatus(const FullBlock& block, Writer<ChainState> pLockedState);

	const Config& m_config;
	std::shared_ptr<Locked<ChainState>> m_pChainState;
};