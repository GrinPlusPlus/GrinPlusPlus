#include "BlockProcessor.h"
#include "BlockHeaderProcessor.h"
#include "../Validators/BlockValidator.h"

#include <Consensus/BlockTime.h>
#include <Infrastructure/Logger.h>
#include <PMMR/HeaderMMR.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <algorithm>

BlockProcessor::BlockProcessor(const Config& config, const IBlockDB& blockDB, ChainState& chainState)
	: m_config(config), m_blockDB(blockDB), m_chainState(chainState)
{

}

EBlockChainStatus BlockProcessor::ProcessBlock(const FullBlock& block)
{	
	const uint64_t candidateHeight = m_chainState.GetHeight(EChainType::CANDIDATE);
	const uint64_t horizonHeight = std::max(candidateHeight, (uint64_t)Consensus::CUT_THROUGH_HORIZON) - Consensus::CUT_THROUGH_HORIZON;

	const BlockHeader& header = block.GetBlockHeader();
	uint64_t height = header.GetHeight();
	if (height <= horizonHeight)
	{
		LoggerAPI::LogWarning("BlockProcessor::ProcessBlock - Can't process blocks beyond horizon.");
		return EBlockChainStatus::INVALID;
	}

	// Make sure header is processed and valid before processing block.
	const EBlockChainStatus headerStatus = BlockHeaderProcessor(m_config, m_chainState).ProcessSingleHeader(header);
	if (headerStatus == EBlockChainStatus::SUCCESS
		|| headerStatus == EBlockChainStatus::ALREADY_EXISTS
		|| headerStatus == EBlockChainStatus::ORPHANED)
	{
		// Verify block is self-consistent before locking
		if (!BlockValidator(m_blockDB, nullptr).IsBlockSelfConsistent(block))
		{
			LoggerAPI::LogWarning("BlockProcessor::ProcessBlock - Failed to validate " + header.FormatHash());
			return EBlockChainStatus::INVALID;
		}

		const EBlockChainStatus returnStatus = ProcessBlockInternal(block);
		if (returnStatus == EBlockChainStatus::SUCCESS)
		{
			LoggerAPI::LogTrace(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s successfully processed.", header.FormatHash().c_str()));
		}

		return returnStatus;
	}

	return headerStatus;
}

EBlockChainStatus BlockProcessor::ProcessBlockInternal(const FullBlock& block)
{
	LockedChainState lockedState = m_chainState.GetLocked();
	Chain& confirmedChain = lockedState.m_chainStore.GetConfirmedChain();
	const BlockHeader& header = block.GetBlockHeader();

	// 1. Check if already part of confirmed chain
	BlockIndex* pConfirmedIndex = confirmedChain.GetByHeight(header.GetHeight());
	if (pConfirmedIndex != nullptr && pConfirmedIndex->GetHash() == header.GetHash())
	{
		LoggerAPI::LogTrace(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s already part of confirmed chain.", header.FormatHash().c_str()));
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 2. Orphan if block should be processed as an orphan
	const EBlockStatus blockStatus = DetermineBlockStatus(block, lockedState);
	if (blockStatus == EBlockStatus::ORPHAN)
	{
		return ProcessOrphanBlock(block, lockedState);
	}

	if (blockStatus == EBlockStatus::REORG)
	{
		return HandleReorg(block, lockedState);
	}
	else
	{
		return ProcessNextBlock(block, lockedState);
	}
}

EBlockChainStatus BlockProcessor::ProcessNextBlock(const FullBlock& block, LockedChainState& lockedState)
{
	const EBlockChainStatus added = ValidateAndAddBlock(block, lockedState);
	if (added != EBlockChainStatus::SUCCESS)
	{
		lockedState.m_txHashSetManager.GetTxHashSet()->Discard();
		return added;
	}

	lockedState.m_txHashSetManager.GetTxHashSet()->Commit();
	lockedState.m_chainStore.AddBlock(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetBlockHeader().GetHeight());
	lockedState.m_chainStore.Flush();

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockProcessor::ProcessOrphanBlock(const FullBlock& block, LockedChainState& lockedState)
{
	// Check if already processed as an orphan
	if (lockedState.m_orphanPool.IsOrphan(block.GetBlockHeader().GetHeight(), block.GetHash()))
	{
		LoggerAPI::LogTrace(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s already processed as an orphan.", block.GetBlockHeader().FormatHash().c_str()));
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	lockedState.m_orphanPool.AddOrphanBlock(block);

	return EBlockChainStatus::ORPHANED;
}

EBlockStatus BlockProcessor::DetermineBlockStatus(const FullBlock& block, LockedChainState& lockedState)
{
	const BlockHeader& header = block.GetBlockHeader();
	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();

	// Orphan if block not a part of candidate chain.
	BlockIndex* pCandidateIndex = candidateChain.GetByHeight(header.GetHeight());
	if (nullptr == pCandidateIndex || pCandidateIndex->GetHash() != header.GetHash())
	{
		LoggerAPI::LogDebug(StringUtil::Format("BlockProcessor::ProcessBlock - Candidate block mismatch. Treating %s as orphan.", header.FormatHash().c_str()));
		return EBlockStatus::ORPHAN;
	}

	Chain& confirmedChain = lockedState.m_chainStore.GetConfirmedChain();

	// Orphan if previous block is missing.
	BlockIndex* pPreviousConfirmedIndex = confirmedChain.GetByHeight(header.GetHeight() - 1);
	if (pPreviousConfirmedIndex == nullptr)
	{
		LoggerAPI::LogTrace(StringUtil::Format("BlockProcessor::ProcessBlock - Previous confirmed block missing. Treating %s as orphan.", header.FormatHash().c_str()));
		return EBlockStatus::ORPHAN;
	}
	
	if (pPreviousConfirmedIndex->GetHash() != header.GetPreviousBlockHash())
	{
		const uint64_t forkPoint = lockedState.m_chainStore.FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight() + 1;

		LoggerAPI::LogWarning(StringUtil::Format("BlockProcessor::ProcessBlock - Fork detected at height (%lld).", forkPoint));

		// If all previous blocks exist (in orphan pool or in block store), return reorg. Otherwise, orphan until they exist.
		for (uint64_t i = forkPoint; i < header.GetHeight(); i++)
		{
			BlockIndex* pIndex = candidateChain.GetByHeight(i);
			if (!lockedState.m_orphanPool.IsOrphan(i, pIndex->GetHash()) && lockedState.m_blockStore.GetBlockByHash(pIndex->GetHash()) == nullptr)
			{
				return EBlockStatus::ORPHAN;
			}
		}

		return EBlockStatus::REORG;
	}

	// Orphan if different block a part of confirmed chain.
	BlockIndex* pConfirmedIndex = confirmedChain.GetByHeight(header.GetHeight());
	if (nullptr != pConfirmedIndex && pConfirmedIndex->GetHash() != header.GetHash())
	{
		LoggerAPI::LogDebug(StringUtil::Format("BlockProcessor::ProcessBlock - Confirmed block mismatch. Treating %s as orphan.", header.FormatHash().c_str()));

		return EBlockStatus::REORG;
	}

	return EBlockStatus::NEXT_BLOCK;
}

EBlockChainStatus BlockProcessor::HandleReorg(const FullBlock& block, LockedChainState& lockedState)
{
	const uint64_t commonHeight = lockedState.m_chainStore.FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight();
	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();

	std::unique_ptr<BlockHeader> pCommonHeader = lockedState.m_blockStore.GetBlockHeaderByHash(candidateChain.GetByHeight(commonHeight)->GetHash());
	if (pCommonHeader == nullptr)
	{
		return EBlockChainStatus::STORE_ERROR;
	}

	// TODO: Add rewound blocks to orphan pool
	// TODO: Add rewound transactions back to TxPool

	ITxHashSet* pTxHashSet = lockedState.m_txHashSetManager.GetTxHashSet();
	if (!pTxHashSet->Rewind(*pCommonHeader))
	{
		pTxHashSet->Discard();
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	for (uint64_t i = commonHeight + 1; i < block.GetBlockHeader().GetHeight(); i++)
	{
		BlockIndex* pIndex = candidateChain.GetByHeight(i);
		std::unique_ptr<FullBlock> pBlock = lockedState.m_orphanPool.GetOrphanBlock(i, pIndex->GetHash());
		if (pBlock == nullptr)
		{
			pBlock = lockedState.m_blockStore.GetBlockByHash(pIndex->GetHash());
		}

		if (pBlock == nullptr)
		{
			pTxHashSet->Discard();
			return EBlockChainStatus::INVALID;
		}

		const EBlockChainStatus added = ValidateAndAddBlock(*pBlock, lockedState);
		if (added != EBlockChainStatus::SUCCESS)
		{
			pTxHashSet->Discard();
			return EBlockChainStatus::INVALID;
		}
	}

	const EBlockChainStatus added = ValidateAndAddBlock(block, lockedState);
	if (added != EBlockChainStatus::SUCCESS)
	{
		pTxHashSet->Discard();
		return EBlockChainStatus::INVALID;
	}

	lockedState.m_chainStore.ReorgChain(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetBlockHeader().GetHeight());
	lockedState.m_chainStore.Flush();
	pTxHashSet->Commit();
	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockProcessor::ValidateAndAddBlock(const FullBlock& block, LockedChainState& lockedState)
{
	std::unique_ptr<BlockHeader> pPreviousHeader = lockedState.m_blockStore.GetBlockHeaderByHash(block.GetBlockHeader().GetPreviousBlockHash());
	if (pPreviousHeader == nullptr)
	{
		return EBlockChainStatus::STORE_ERROR;
	}

	ITxHashSet* pTxHashSet = lockedState.m_txHashSetManager.GetTxHashSet();

	if (!pTxHashSet->ApplyBlock(block))
	{
		return EBlockChainStatus::INVALID;
	}

	std::unique_ptr<BlockSums> pBlockSums = BlockValidator(m_blockDB, pTxHashSet).ValidateBlock(block);
	if (pBlockSums == nullptr)
	{
		return EBlockChainStatus::INVALID;
	}

	lockedState.m_blockStore.GetBlockDB().AddBlockSums(block.GetHash(), *pBlockSums);
	lockedState.m_blockStore.AddBlock(block);
	lockedState.m_orphanPool.RemoveOrphan(block.GetBlockHeader().GetHeight(), block.GetHash());
	lockedState.m_transactionPool.ReconcileBlock(block);

	return EBlockChainStatus::SUCCESS;
}