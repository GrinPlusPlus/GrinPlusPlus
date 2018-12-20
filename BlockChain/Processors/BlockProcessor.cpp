#include "BlockProcessor.h"
#include "BlockHeaderProcessor.h"
#include "../Validators/BlockValidator.h"

#include <Consensus/BlockTime.h>
#include <Infrastructure/Logger.h>
#include <HeaderMMR.h>
#include <HexUtil.h>
#include <StringUtil.h>
#include <algorithm>

BlockProcessor::BlockProcessor(ChainState& chainState)
	: m_chainState(chainState)
{

}

EBlockChainStatus BlockProcessor::ProcessBlock(const FullBlock& block)
{	
	const uint64_t candidateHeight = m_chainState.GetHeight(EChainType::CANDIDATE);
	const uint64_t horizonHeight = std::max(candidateHeight, (uint64_t)Consensus::CUT_THROUGH_HORIZON) - Consensus::CUT_THROUGH_HORIZON;

	const BlockHeader& header = block.GetBlockHeader();
	const uint64_t height = header.GetHeight();
	if (height <= horizonHeight)
	{
		LoggerAPI::LogError("BlockProcessor::ProcessBlock - Can't process blocks beyond horizon.");
		return EBlockChainStatus::INVALID;
	}

	// Make sure header is processed and valid before processing block.
	const EBlockChainStatus headerStatus = BlockHeaderProcessor(m_chainState).ProcessSingleHeader(header);
	if (headerStatus == EBlockChainStatus::SUCCESS
		|| headerStatus == EBlockChainStatus::ALREADY_EXISTS
		|| headerStatus == EBlockChainStatus::ORPHANED)
	{
		return ProcessBlockInternal(block);
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
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s already part of confirmed chain.", header.FormatHash().c_str()));
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 2. Check if already processed as an orphan
	if (lockedState.m_orphanPool.IsOrphan(header.GetHash()))
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s already processed as an orphan.", header.FormatHash().c_str()));
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 3. Orphan if block should be processed as an orphan
	if (ShouldOrphan(block, lockedState.m_chainStore.GetCandidateChain()))
	{
		return ProcessOrphanBlock(block, lockedState);
	}

	return ProcessNextBlock(block, lockedState);
}

EBlockChainStatus BlockProcessor::ProcessNextBlock(const FullBlock& block, LockedChainState& lockedState)
{
	// TODO: Remove from orphanPool (if there) and check for orphans to process

	Chain& confirmedChain = lockedState.m_chainStore.GetCandidateChain();

	confirmedChain.Rewind(block.GetBlockHeader().GetHeight() - 1);

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockProcessor::ProcessOrphanBlock(const FullBlock& block, LockedChainState& lockedState)
{
	// TODO: Implement
	return EBlockChainStatus::ORPHANED;
}

bool BlockProcessor::ShouldOrphan(const FullBlock& block, Chain& candidateChain)
{
	// Orphan if previous header not a part of candidate chain.
	const BlockHeader& header = block.GetBlockHeader();
	BlockIndex* pPreviousIndex = candidateChain.GetByHeight(header.GetHeight() - 1);
	if (pPreviousIndex == nullptr || pPreviousIndex->GetHash() != header.GetPreviousBlockHash())
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Previous header missing. Treating %s as orphan.", header.FormatHash().c_str()));

		return true;
	}

	// Orphan if header not a part of candidate chain.
	BlockIndex* pCandidateIndex = candidateChain.GetByHeight(header.GetHeight());
	if (nullptr == pCandidateIndex || pCandidateIndex->GetHash() != header.GetHash())
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Candidate header mismatch. Treating %s as orphan.", header.FormatHash().c_str()));

		return true;
	}

	return false;
}