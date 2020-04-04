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

	BlockHeaderPtr pHeader = block.GetBlockHeader();
	if (pHeader->GetHeight() <= horizonHeight)
	{
		LOG_WARNING("Can't process blocks beyond horizon.");
		throw BAD_DATA_EXCEPTION("Can't process blocks beyond horizon.");
	}

	// Make sure header is processed and valid before processing block.
	const EBlockChainStatus headerStatus = BlockHeaderProcessor(m_config, m_pChainState).ProcessSingleHeader(pHeader);
	if (headerStatus == EBlockChainStatus::SUCCESS
		|| headerStatus == EBlockChainStatus::ALREADY_EXISTS
		|| headerStatus == EBlockChainStatus::ORPHANED)
	{
		// Verify block is self-consistent before locking
		BlockValidator(m_config, m_pChainState->Read()->GetBlockDB().GetShared(), nullptr).VerifySelfConsistent(block);

		const EBlockChainStatus returnStatus = ProcessBlockInternal(block);
		if (returnStatus == EBlockChainStatus::SUCCESS)
		{
			LOG_TRACE_F("Block {} successfully processed.", *pHeader);
		}

		return returnStatus;
	}

	return headerStatus;
}

EBlockChainStatus BlockProcessor::ProcessBlockInternal(const FullBlock& block)
{
	auto pBatch = m_pChainState->BatchWrite();
	auto pChainStore = pBatch->GetChainStore();
	auto pOrphanPool = pBatch->GetOrphanPool();
	auto pConfirmedChain = pChainStore->GetConfirmedChain();

	// 1. Check if already part of confirmed chain
	std::shared_ptr<const BlockIndex> pConfirmedIndex = pConfirmedChain->GetByHeight(block.GetHeight());
	if (pConfirmedIndex != nullptr && pConfirmedIndex->GetHash() == block.GetHash())
	{
		LOG_TRACE_F("Block {} already part of confirmed chain.", block);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 2. Orphan if block should be processed as an orphan
	const EBlockStatus blockStatus = DetermineBlockStatus(block, pBatch);
	switch (blockStatus)
	{
		case EBlockStatus::ORPHAN:
		{
			if (pOrphanPool->IsOrphan(block.GetHeight(), block.GetHash()))
			{
				LOG_TRACE_F("Block {} already processed as an orphan.", block);
				return EBlockChainStatus::ALREADY_EXISTS;
			}

			pOrphanPool->AddOrphanBlock(block);
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

			pChainStore->AddBlock(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetHeight());
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
	auto pOrphanPool = pBatch->GetOrphanPool();
	auto pBlockDB = pBatch->GetBlockDB();
	auto pChainStore = pBatch->GetChainStore();
	auto pCandidateChain = pChainStore->GetCandidateChain();
	auto pConfirmedChain = pChainStore->GetConfirmedChain();

	// Orphan if block not a part of candidate chain.
	auto pCandidateIndex = pCandidateChain->GetByHeight(block.GetHeight());
	if (nullptr == pCandidateIndex || pCandidateIndex->GetHash() != block.GetHash())
	{
		LOG_DEBUG_F("Candidate block mismatch. Treating {} as orphan.", block);
		return EBlockStatus::ORPHAN;
	}

	// Orphan if previous block is missing.
	auto pPreviousConfirmedIndex = pConfirmedChain->GetByHeight(block.GetHeight() - 1);
	if (pPreviousConfirmedIndex == nullptr)
	{
		LOG_TRACE_F("Previous confirmed block missing. Treating {} as orphan.", block);
		return EBlockStatus::ORPHAN;
	}
	
	if (pPreviousConfirmedIndex->GetHash() != block.GetPreviousHash())
	{
		const uint64_t forkPoint = pChainStore->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight() + 1;

		LOG_WARNING_F("Fork detected at height {}.", forkPoint);

		// If all previous blocks exist (in orphan pool or in block store), return reorg. Otherwise, orphan until they exist.
		for (uint64_t i = forkPoint; i < block.GetHeight(); i++)
		{
			std::shared_ptr<const BlockIndex> pIndex = pCandidateChain->GetByHeight(i);
			if (!pOrphanPool->IsOrphan(i, pIndex->GetHash()) && pBlockDB->GetBlock(pIndex->GetHash()) == nullptr)
			{
				return EBlockStatus::ORPHAN;
			}
		}

		return EBlockStatus::REORG;
	}

	// Orphan if different block a part of confirmed chain.
	std::shared_ptr<const BlockIndex> pConfirmedIndex = pConfirmedChain->GetByHeight(block.GetHeight());
	if (nullptr != pConfirmedIndex && pConfirmedIndex->GetHash() != block.GetHash())
	{
		LOG_DEBUG_F("Confirmed block mismatch. Treating {} as orphan.", block);

		return EBlockStatus::REORG;
	}

	return EBlockStatus::NEXT_BLOCK;
}

void BlockProcessor::HandleReorg(const FullBlock& block, Writer<ChainState> pBatch)
{
	auto pOrphanPool = pBatch->GetOrphanPool();
	auto pBlockDB = pBatch->GetBlockDB();
	auto pChainStore = pBatch->GetChainStore();
	auto pCandidateChain = pChainStore->GetCandidateChain();
	auto pConfirmedChain = pChainStore->GetConfirmedChain();
	auto pTxHashSet = pBatch->GetTxHashSetManager()->GetTxHashSet();

	const uint64_t commonHeight = pChainStore->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight();

	const Hash& commonHash = pCandidateChain->GetByHeight(commonHeight)->GetHash();
	auto pCommonHeader = pBlockDB->GetBlockHeader(commonHash);
	if (pCommonHeader == nullptr)
	{
		LOG_ERROR_F("Failed to find header {}", commonHash);
		throw BLOCK_CHAIN_EXCEPTION("Failed to find header.");
	}

	// TODO: Add rewound blocks to orphan pool
	// TODO: Add rewound transactions back to TxPool

	if (pTxHashSet == nullptr)
	{
		LOG_ERROR("TxHashSet is null");
		throw BLOCK_CHAIN_EXCEPTION("TxHashSet is null");
	}

	pTxHashSet->Rewind(pBlockDB, *pCommonHeader);

	for (uint64_t i = commonHeight + 1; i < block.GetHeight(); i++)
	{
		auto pIndex = pCandidateChain->GetByHeight(i);
		auto pBlock = pOrphanPool->GetOrphanBlock(i, pIndex->GetHash());
		if (pBlock == nullptr)
		{
			pBlock = pBlockDB->GetBlock(pIndex->GetHash());
		}

		if (pBlock == nullptr)
		{
			LOG_ERROR_F("Failed to find block {}", pIndex->GetHash());
			throw BAD_DATA_EXCEPTION("Missing block");
		}

		ValidateAndAddBlock(*pBlock, pBatch);
	}

	ValidateAndAddBlock(block, pBatch);

	pChainStore->ReorgChain(EChainType::CANDIDATE, EChainType::CONFIRMED, block.GetHeight());
}

void BlockProcessor::ValidateAndAddBlock(const FullBlock& block, Writer<ChainState> pBatch)
{
	auto pOrphanPool = pBatch->GetOrphanPool();
	auto pBlockDB = pBatch->GetBlockDB();
	auto pTxHashSet = pBatch->GetTxHashSetManager()->GetTxHashSet();
	auto pTxPool = pBatch->GetTransactionPool();

	const Hash& previousHash = block.GetPreviousHash();
	auto pPreviousHeader = pBlockDB->GetBlockHeader(previousHash);
	if (pPreviousHeader == nullptr)
	{
		LOG_ERROR_F("Failed to find header {}", previousHash);
		throw BLOCK_CHAIN_EXCEPTION("Previous header not found.");
	}

	if (pTxHashSet == nullptr || !pTxHashSet->ApplyBlock(pBlockDB, block))
	{
		LOG_ERROR_F("Failed to apply block {} to the TxHashSet", block);
		throw BAD_DATA_EXCEPTION("Failed to apply block to the TxHashSet.");
	}

	BlockSums blockSums = BlockValidator(m_config, pBlockDB, pTxHashSet).ValidateBlock(block);

	pBlockDB->RemoveOutputPositions(block.GetInputCommitments());
	pBlockDB->AddBlockSums(block.GetHash(), blockSums);
	pBlockDB->AddBlock(block);
	pOrphanPool->RemoveOrphan(block.GetHeight(), block.GetHash());
	pTxPool->ReconcileBlock(pBlockDB, pTxHashSet, block);
}