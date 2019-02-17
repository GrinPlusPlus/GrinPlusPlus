#pragma once

#include "../ChainState.h"

#include <Config/Config.h>
#include <Core/Models/FullBlock.h>
#include <BlockChain/BlockChainStatus.h>
#include <TxPool/TransactionPool.h>

// TODO: Move this?
enum class EBlockStatus
{
	NEXT_BLOCK,
	REORG,
	ORPHAN
};

class BlockProcessor
{
public:
	BlockProcessor(const Config& config, const IBlockDB& blockDB, ChainState& chainState, const ITransactionPool& transactionPool);

	EBlockChainStatus ProcessBlock(const FullBlock& block, const bool orphan);

private:
	EBlockChainStatus ProcessBlockInternal(const FullBlock& block);
	EBlockChainStatus ProcessNextBlock(const FullBlock& block, LockedChainState& lockedState);
	EBlockChainStatus ProcessOrphanBlock(const FullBlock& block, LockedChainState& lockedState);
	EBlockChainStatus HandleReorg(const FullBlock& block, LockedChainState& lockedState);
	EBlockChainStatus ValidateAndAddBlock(const FullBlock& block, LockedChainState& lockedState);

	EBlockStatus DetermineBlockStatus(const FullBlock& block, LockedChainState& lockedState);

	const Config& m_config;
	const IBlockDB& m_blockDB;
	ChainState& m_chainState;
	const ITransactionPool& m_transactionPool;
};