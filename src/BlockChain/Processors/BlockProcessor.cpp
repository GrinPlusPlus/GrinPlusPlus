#include "BlockProcessor.h"
#include "BlockHeaderProcessor.h"
#include "../Validators/BlockValidator.h"

#include <Consensus/BlockTime.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <algorithm>

BlockProcessor::BlockProcessor(const Config& config, std::shared_ptr<Locked<ChainState>> pChainState)
	: m_config(config), m_pChainState(pChainState)
{

}

EBlockChainStatus BlockProcessor::ProcessBlock(const FullBlock& block)
{	
	const uint64_t candidateHeight = m_pChainState->Read()->GetHeight(EChainType::CANDIDATE);
	const uint64_t horizonHeight = Consensus::GetHorizonHeight(candidateHeight);

	const BlockHeader& header = block.GetBlockHeader();
	uint64_t height = header.GetHeight();
	if (height <= horizonHeight)
	{
		LOG_WARNING("Can't process blocks beyond horizon.");
		return EBlockChainStatus::INVALID;
	}

	// Make sure header is processed and valid before processing block.
	const EBlockChainStatus headerStatus = BlockHeaderProcessor(m_config, m_pChainState).ProcessSingleHeader(header);
	if (headerStatus == EBlockChainStatus::SUCCESS
		|| headerStatus == EBlockChainStatus::ALREADY_EXISTS
		|| headerStatus == EBlockChainStatus::ORPHANED)
	{
		// Verify block is self-consistent before locking
		if (!BlockValidator(m_pChainState->Read()->GetBlockDB().GetShared(), nullptr).IsBlockSelfConsistent(block))
		{
			LOG_WARNING_F("Failed to validate %s", header);
			return EBlockChainStatus::INVALID;
		}

		const EBlockChainStatus returnStatus = ProcessBlockInternal(block);
		if (returnStatus == EBlockChainStatus::SUCCESS)
		{
			LOG_TRACE_F("Block %s successfully processed.", header);
		}

		return returnStatus;
	}

	return headerStatus;
}

EBlockChainStatus BlockProcessor::ProcessBlockInternal(const FullBlock& block)
{
	auto pLockedState = m_pChainState->BatchWrite();

	std::shared_ptr<Chain> pConfirmedChain = pLockedState->GetChainStore()->GetConfirmedChain();
	const BlockHeader& header = block.GetBlockHeader();

	// 1. Check if already part of confirmed chain
	BlockIndex* pConfirmedIndex = pConfirmedChain->GetByHeight(header.GetHeight());
	if (pConfirmedIndex != nullptr && pConfirmedIndex->GetHash() == header.GetHash())
	{
		LOG_TRACE_F("Block %s already part of confirmed chain.", header);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 2. Orphan if block should be processed as an orphan
	const EBlockStatus blockStatus = DetermineBlockStatus(block, pLockedState);
	if (blockStatus == EBlockStatus::ORPHAN)
	{
		const EBlockChainStatus status = ProcessOrphanBlock(block, pLockedState);
		pLockedState->Commit();
		return status;
	}

	EBlockChainStatus status = EBlockChainStatus::SUCCESS;
	if (blockStatus == EBlockStatus::REORG)
	{
		status = HandleReorg(block, pLockedState);
	}
	else
	{
		status = ProcessNextBlock(block, pLockedState);
	}

	if (status == EBlockChainStatus::SUCCESS)
	{
		pLockedState->Commit();
	}

	return status;
}

EBlockChainStatus BlockProcessor::ProcessNextBlock(const FullBlock& block, Writer<ChainState> pLockedState)
{
	const EBlockChainStatus added = ValidateAndAddBlock(block, pLockedState);
	if (added != EBlockChainStatus::SUCCESS)
	{
		pLockedState->GetTxHashSetManager()->GetTxHashSet()->Rollback();
		return added;
	}

	pLockedState->GetTxHashSetManager()->GetTxHashSet()->Commit();
	pLockedState->GetChainStore()->AddBlock(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetBlockHeader().GetHeight());
	pLockedState->GetChainStore()->Commit();

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockProcessor::ProcessOrphanBlock(const FullBlock& block, Writer<ChainState> pLockedState)
{
	// Check if already processed as an orphan
	if (pLockedState->GetOrphanPool()->IsOrphan(block.GetBlockHeader().GetHeight(), block.GetHash()))
	{
		LOG_TRACE_F("Block %s already processed as an orphan.", block);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	pLockedState->GetOrphanPool()->AddOrphanBlock(block);

	return EBlockChainStatus::ORPHANED;
}

EBlockStatus BlockProcessor::DetermineBlockStatus(const FullBlock& block, Writer<ChainState> pLockedState)
{
	const BlockHeader& header = block.GetBlockHeader();
	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	// Orphan if block not a part of candidate chain.
	BlockIndex* pCandidateIndex = pCandidateChain->GetByHeight(header.GetHeight());
	if (nullptr == pCandidateIndex || pCandidateIndex->GetHash() != header.GetHash())
	{
		LOG_DEBUG_F("Candidate block mismatch. Treating %s as orphan.", header);
		return EBlockStatus::ORPHAN;
	}

	std::shared_ptr<Chain> pConfirmedChain = pLockedState->GetChainStore()->GetConfirmedChain();

	// Orphan if previous block is missing.
	BlockIndex* pPreviousConfirmedIndex = pConfirmedChain->GetByHeight(header.GetHeight() - 1);
	if (pPreviousConfirmedIndex == nullptr)
	{
		LOG_TRACE_F("Previous confirmed block missing. Treating %s as orphan.", header);
		return EBlockStatus::ORPHAN;
	}
	
	if (pPreviousConfirmedIndex->GetHash() != header.GetPreviousBlockHash())
	{
		const uint64_t forkPoint = pLockedState->GetChainStore()->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight() + 1;

		LOG_WARNING_F("Fork detected at height (%lld).", forkPoint);

		// If all previous blocks exist (in orphan pool or in block store), return reorg. Otherwise, orphan until they exist.
		for (uint64_t i = forkPoint; i < header.GetHeight(); i++)
		{
			BlockIndex* pIndex = pCandidateChain->GetByHeight(i);
			if (!pLockedState->GetOrphanPool()->IsOrphan(i, pIndex->GetHash()) && pLockedState->GetBlockDB()->GetBlock(pIndex->GetHash()) == nullptr)
			{
				return EBlockStatus::ORPHAN;
			}
		}

		return EBlockStatus::REORG;
	}

	// Orphan if different block a part of confirmed chain.
	BlockIndex* pConfirmedIndex = pConfirmedChain->GetByHeight(header.GetHeight());
	if (nullptr != pConfirmedIndex && pConfirmedIndex->GetHash() != header.GetHash())
	{
		LOG_DEBUG_F("Confirmed block mismatch. Treating %s as orphan.", header);

		return EBlockStatus::REORG;
	}

	return EBlockStatus::NEXT_BLOCK;
}

EBlockChainStatus BlockProcessor::HandleReorg(const FullBlock& block, Writer<ChainState> pLockedState)
{
	const uint64_t commonHeight = pLockedState->GetChainStore()->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight();
	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	std::unique_ptr<BlockHeader> pCommonHeader = pLockedState->GetBlockDB()->GetBlockHeader(pCandidateChain->GetByHeight(commonHeight)->GetHash());
	if (pCommonHeader == nullptr)
	{
		return EBlockChainStatus::STORE_ERROR;
	}

	// TODO: Add rewound blocks to orphan pool
	// TODO: Add rewound transactions back to TxPool

	ITxHashSetPtr pTxHashSet = pLockedState->GetTxHashSetManager()->GetTxHashSet();
	if (!pTxHashSet->Rewind(pLockedState->GetBlockDB(), *pCommonHeader))
	{
		pTxHashSet->Rollback();
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	for (uint64_t i = commonHeight + 1; i < block.GetBlockHeader().GetHeight(); i++)
	{
		BlockIndex* pIndex = pCandidateChain->GetByHeight(i);
		std::unique_ptr<FullBlock> pBlock = pLockedState->GetOrphanPool()->GetOrphanBlock(i, pIndex->GetHash());
		if (pBlock == nullptr)
		{
			pBlock = pLockedState->GetBlockDB()->GetBlock(pIndex->GetHash());
		}

		if (pBlock == nullptr)
		{
			pTxHashSet->Rollback();
			return EBlockChainStatus::INVALID;
		}

		const EBlockChainStatus added = ValidateAndAddBlock(*pBlock, pLockedState);
		if (added != EBlockChainStatus::SUCCESS)
		{
			pTxHashSet->Rollback();
			return EBlockChainStatus::INVALID;
		}
	}

	const EBlockChainStatus added = ValidateAndAddBlock(block, pLockedState);
	if (added != EBlockChainStatus::SUCCESS)
	{
		pTxHashSet->Rollback();
		return EBlockChainStatus::INVALID;
	}

	pLockedState->GetChainStore()->ReorgChain(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetBlockHeader().GetHeight());
	pLockedState->GetChainStore()->Commit();
	pTxHashSet->Commit();
	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockProcessor::ValidateAndAddBlock(const FullBlock& block, Writer<ChainState> pLockedState)
{
	std::unique_ptr<BlockHeader> pPreviousHeader = pLockedState->GetBlockDB()->GetBlockHeader(block.GetBlockHeader().GetPreviousBlockHash());
	if (pPreviousHeader == nullptr)
	{
		return EBlockChainStatus::STORE_ERROR;
	}

	ITxHashSetPtr pTxHashSet = pLockedState->GetTxHashSetManager()->GetTxHashSet();

	if (!pTxHashSet->ApplyBlock(pLockedState->GetBlockDB(), block))
	{
		return EBlockChainStatus::INVALID;
	}

	std::unique_ptr<BlockSums> pBlockSums = BlockValidator(pLockedState->GetBlockDB(), pTxHashSet).ValidateBlock(block);
	if (pBlockSums == nullptr)
	{
		return EBlockChainStatus::INVALID;
	}

	pLockedState->GetBlockDB()->AddBlockSums(block.GetHash(), *pBlockSums);
	pLockedState->GetBlockDB()->AddBlock(block);
	pLockedState->GetOrphanPool()->RemoveOrphan(block.GetBlockHeader().GetHeight(), block.GetHash());
	pLockedState->GetTransactionPool()->ReconcileBlock(pLockedState->GetBlockDB(), block);

	return EBlockChainStatus::SUCCESS;
}