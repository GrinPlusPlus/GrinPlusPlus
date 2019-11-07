#include "BlockProcessor.h"
#include "BlockHeaderProcessor.h"
#include "../Validators/BlockValidator.h"

#include <Core/Exceptions/BlockChainException.h>
#include <Core/Exceptions/BadDataException.h>
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
	if (header.GetHeight() <= horizonHeight)
	{
		LOG_WARNING("Can't process blocks beyond horizon.");
		throw BAD_DATA_EXCEPTION("Can't process blocks beyond horizon.");
	}

	// Make sure header is processed and valid before processing block.
	const EBlockChainStatus headerStatus = BlockHeaderProcessor(m_config, m_pChainState).ProcessSingleHeader(header);
	if (headerStatus == EBlockChainStatus::SUCCESS
		|| headerStatus == EBlockChainStatus::ALREADY_EXISTS
		|| headerStatus == EBlockChainStatus::ORPHANED)
	{
		// Verify block is self-consistent before locking
		BlockValidator(m_pChainState->Read()->GetBlockDB().GetShared(), nullptr).VerifySelfConsistent(block);

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
	auto pBatch = m_pChainState->BatchWrite();

	std::shared_ptr<Chain> pConfirmedChain = pBatch->GetChainStore()->GetConfirmedChain();
	const BlockHeader& header = block.GetBlockHeader();

	// 1. Check if already part of confirmed chain
	std::shared_ptr<const BlockIndex> pConfirmedIndex = pConfirmedChain->GetByHeight(header.GetHeight());
	if (pConfirmedIndex != nullptr && pConfirmedIndex->GetHash() == header.GetHash())
	{
		LOG_TRACE_F("Block %s already part of confirmed chain.", header);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 2. Orphan if block should be processed as an orphan
	const EBlockStatus blockStatus = DetermineBlockStatus(block, pBatch);
	switch (blockStatus)
	{
		case EBlockStatus::ORPHAN:
		{
			if (pBatch->GetOrphanPool()->IsOrphan(block.GetHeight(), block.GetHash()))
			{
				LOG_TRACE_F("Block %s already processed as an orphan.", block);
				return EBlockChainStatus::ALREADY_EXISTS;
			}

			pBatch->GetOrphanPool()->AddOrphanBlock(block);
			pBatch->Commit();

			return EBlockChainStatus::ORPHANED;
		}
		case EBlockStatus::REORG:
		{
			HandleReorg(block, pBatch);
			pBatch->Commit();

			return EBlockChainStatus::SUCCESS;
		}
		case EBlockStatus::NEXT_BLOCK:
		{
			ValidateAndAddBlock(block, pBatch);

			pBatch->GetChainStore()->AddBlock(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetBlockHeader().GetHeight());
			pBatch->Commit();

			return EBlockChainStatus::SUCCESS;
		}
		default:
		{
			throw BLOCK_CHAIN_EXCEPTION("Unexpected block status returned");
		}
	}
}

EBlockStatus BlockProcessor::DetermineBlockStatus(const FullBlock& block, Writer<ChainState> pBatch)
{
	const BlockHeader& header = block.GetBlockHeader();
	std::shared_ptr<Chain> pCandidateChain = pBatch->GetChainStore()->GetCandidateChain();

	// Orphan if block not a part of candidate chain.
	std::shared_ptr<const BlockIndex> pCandidateIndex = pCandidateChain->GetByHeight(header.GetHeight());
	if (nullptr == pCandidateIndex || pCandidateIndex->GetHash() != header.GetHash())
	{
		LOG_DEBUG_F("Candidate block mismatch. Treating %s as orphan.", header);
		return EBlockStatus::ORPHAN;
	}

	std::shared_ptr<Chain> pConfirmedChain = pBatch->GetChainStore()->GetConfirmedChain();

	// Orphan if previous block is missing.
	std::shared_ptr<const BlockIndex> pPreviousConfirmedIndex = pConfirmedChain->GetByHeight(header.GetHeight() - 1);
	if (pPreviousConfirmedIndex == nullptr)
	{
		LOG_TRACE_F("Previous confirmed block missing. Treating %s as orphan.", header);
		return EBlockStatus::ORPHAN;
	}
	
	if (pPreviousConfirmedIndex->GetHash() != header.GetPreviousBlockHash())
	{
		const uint64_t forkPoint = pBatch->GetChainStore()->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight() + 1;

		LOG_WARNING_F("Fork detected at height (%lld).", forkPoint);

		// If all previous blocks exist (in orphan pool or in block store), return reorg. Otherwise, orphan until they exist.
		for (uint64_t i = forkPoint; i < header.GetHeight(); i++)
		{
			std::shared_ptr<const BlockIndex> pIndex = pCandidateChain->GetByHeight(i);
			if (!pBatch->GetOrphanPool()->IsOrphan(i, pIndex->GetHash()) && pBatch->GetBlockDB()->GetBlock(pIndex->GetHash()) == nullptr)
			{
				return EBlockStatus::ORPHAN;
			}
		}

		return EBlockStatus::REORG;
	}

	// Orphan if different block a part of confirmed chain.
	std::shared_ptr<const BlockIndex> pConfirmedIndex = pConfirmedChain->GetByHeight(header.GetHeight());
	if (nullptr != pConfirmedIndex && pConfirmedIndex->GetHash() != header.GetHash())
	{
		LOG_DEBUG_F("Confirmed block mismatch. Treating %s as orphan.", header);

		return EBlockStatus::REORG;
	}

	return EBlockStatus::NEXT_BLOCK;
}

void BlockProcessor::HandleReorg(const FullBlock& block, Writer<ChainState> pBatch)
{
	const uint64_t commonHeight = pBatch->GetChainStore()->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight();
	std::shared_ptr<Chain> pCandidateChain = pBatch->GetChainStore()->GetCandidateChain();

	const Hash& commonHash = pCandidateChain->GetByHeight(commonHeight)->GetHash();
	std::unique_ptr<BlockHeader> pCommonHeader = pBatch->GetBlockDB()->GetBlockHeader(commonHash);
	if (pCommonHeader == nullptr)
	{
		LOG_ERROR_F("Failed to find header %s", commonHash);
		throw BLOCK_CHAIN_EXCEPTION("Failed to find header.");
	}

	// TODO: Add rewound blocks to orphan pool
	// TODO: Add rewound transactions back to TxPool

	ITxHashSetPtr pTxHashSet = pBatch->GetTxHashSet();
	if (pTxHashSet == nullptr || !pTxHashSet->Rewind(pBatch->GetBlockDB(), *pCommonHeader))
	{
		LOG_ERROR_F("Failed to rewind TxHashSet to block %s", pCommonHeader->GetHash());
		throw BLOCK_CHAIN_EXCEPTION("Failed to rewind TxHashSet");
	}

	for (uint64_t i = commonHeight + 1; i < block.GetBlockHeader().GetHeight(); i++)
	{
		std::shared_ptr<const BlockIndex> pIndex = pCandidateChain->GetByHeight(i);
		std::unique_ptr<FullBlock> pBlock = pBatch->GetOrphanPool()->GetOrphanBlock(i, pIndex->GetHash());
		if (pBlock == nullptr)
		{
			pBlock = pBatch->GetBlockDB()->GetBlock(pIndex->GetHash());
		}

		if (pBlock == nullptr)
		{
			LOG_ERROR_F("Failed to find block %s", pIndex->GetHash());
			throw BAD_DATA_EXCEPTION("Missing block");
		}

		ValidateAndAddBlock(*pBlock, pBatch);
	}

	ValidateAndAddBlock(block, pBatch);

	pBatch->GetChainStore()->ReorgChain(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetBlockHeader().GetHeight());
}

void BlockProcessor::ValidateAndAddBlock(const FullBlock& block, Writer<ChainState> pBatch)
{
	const Hash& previousHash = block.GetBlockHeader().GetPreviousBlockHash();
	std::unique_ptr<BlockHeader> pPreviousHeader = pBatch->GetBlockDB()->GetBlockHeader(previousHash);
	if (pPreviousHeader == nullptr)
	{
		LOG_ERROR_F("Failed to find header %s", previousHash);
		throw BLOCK_CHAIN_EXCEPTION("Previous header not found.");
	}

	ITxHashSetPtr pTxHashSet = pBatch->GetTxHashSet();

	if (pTxHashSet == nullptr || !pTxHashSet->ApplyBlock(pBatch->GetBlockDB(), block))
	{
		LOG_ERROR_F("Failed to apply block %s to the TxHashSet", block);
		throw BAD_DATA_EXCEPTION("Failed to apply block to the TxHashSet.");
	}

	BlockSums blockSums = BlockValidator(pBatch->GetBlockDB(), pTxHashSet).ValidateBlock(block);

	pBatch->GetBlockDB()->AddBlockSums(block.GetHash(), blockSums);
	pBatch->GetBlockDB()->AddBlock(block);
	pBatch->GetOrphanPool()->RemoveOrphan(block.GetBlockHeader().GetHeight(), block.GetHash());
	pBatch->GetTransactionPool()->ReconcileBlock(pBatch->GetBlockDB(), pTxHashSet, block);
}