#pragma once

#include "../ChainState.h"

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
	struct BlockProcessingInfo
	{
		EBlockStatus status;
		std::vector<FullBlock::CPtr> reorgBlocks;
	};
public:
	BlockProcessor(const std::shared_ptr<Locked<ChainState>>& pChainState);

	EBlockChainStatus ProcessBlock(const FullBlock& block);

private:
	EBlockChainStatus ProcessBlockInternal(const FullBlock& block);
	void HandleReorg(Writer<ChainState> pBatch, const std::vector<FullBlock::CPtr>& reorgBlocks);
	void ValidateAndAddBlock(const FullBlock& block, Writer<ChainState> pLockedState);

	BlockProcessingInfo DetermineBlockStatus(const FullBlock& block, Writer<ChainState> pLockedState);

	std::shared_ptr<Locked<ChainState>> m_pChainState;
};