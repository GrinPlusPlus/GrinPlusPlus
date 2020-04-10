#include "BlockProcessor.h"
#include "BlockHeaderProcessor.h"
#include "../Validators/BlockValidator.h"
#include "../Validators/ContextualBlockValidator.h"

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
	const EBlockChainStatus headerStatus = BlockHeaderProcessor(m_config, m_pChainState).ProcessSingleHeader(pHeader); // TODO: Can probably ignore status, as long as no exceptions
	if (headerStatus == EBlockChainStatus::SUCCESS
		|| headerStatus == EBlockChainStatus::ALREADY_EXISTS
		|| headerStatus == EBlockChainStatus::ORPHANED)
	{
		// Verify block is self-consistent before locking
		BlockValidator::VerifySelfConsistent(block);

		const EBlockChainStatus returnStatus = ProcessBlockInternal(block);
		if (returnStatus == EBlockChainStatus::SUCCESS)
		{
			LOG_DEBUG_F("Block {} successfully processed.", *pHeader);
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
	if (pConfirmedChain->IsOnChain(block.GetHeader()))
	{
		LOG_TRACE_F("Block {} already part of confirmed chain.", block);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// TODO: Check if in BlockDB

	// 2. Orphan if block should be processed as an orphan
	const BlockProcessingInfo info = DetermineBlockStatus(block, pBatch);
	if (info.status == EBlockStatus::ORPHAN)
	{
		if (pOrphanPool->IsOrphan(block.GetHeight(), block.GetHash()))
		{
			LOG_TRACE_F("Block {} already processed as an orphan.", block);
			return EBlockChainStatus::ALREADY_EXISTS;
		}

		pOrphanPool->AddOrphanBlock(block);

		return EBlockChainStatus::ORPHANED;
	}
	else if (info.status == EBlockStatus::REORG)
	{
		HandleReorg(pBatch, info.reorgBlocks);
		return EBlockChainStatus::SUCCESS;
	}
	else
	{
		assert(info.status == EBlockStatus::NEXT_BLOCK);

		ValidateAndAddBlock(block, pBatch);
		pConfirmedChain->AddBlock(block.GetHash());
		pBatch->Commit();

		return EBlockChainStatus::SUCCESS;
	}

	//switch (info.status)
	//{
	//	case EBlockStatus::ORPHAN:
	//	{
	//		if (pOrphanPool->IsOrphan(block.GetHeight(), block.GetHash()))
	//		{
	//			LOG_TRACE_F("Block {} already processed as an orphan.", block);
	//			return EBlockChainStatus::ALREADY_EXISTS;
	//		}

	//		pOrphanPool->AddOrphanBlock(block);

	//		return EBlockChainStatus::ORPHANED;
	//	}
	//	case EBlockStatus::REORG:
	//	{
	//		HandleReorg(block, pBatch);
	//		pBatch->Commit();

	//		return EBlockChainStatus::SUCCESS;
	//	}
	//	case EBlockStatus::NEXT_BLOCK:
	//	{
	//		ValidateAndAddBlock(block, pBatch);
	//		pConfirmedChain->AddBlock(block.GetHash());
	//		pBatch->Commit();

	//		return EBlockChainStatus::SUCCESS;
	//	}
	//	default:
	//	{
	//		throw BLOCK_CHAIN_EXCEPTION("Unexpected block status returned");
	//	}
	//}
}

BlockProcessor::BlockProcessingInfo BlockProcessor::DetermineBlockStatus(const FullBlock& block, Writer<ChainState> pBatch)
{
	auto pOrphanPool = pBatch->GetOrphanPool();
	auto pBlockDB = pBatch->GetBlockDB();
	auto pChainStore = pBatch->GetChainStore();
	auto pCandidateChain = pChainStore->GetCandidateChain();
	auto pConfirmedChain = pChainStore->GetConfirmedChain();

	auto pStoreHeader = pBlockDB->GetBlockHeader(block.GetHash());
	if (pStoreHeader == nullptr)
	{
		LOG_TRACE_F("Header has not yet been validated. Treating {} as orphan.", block);
		return { EBlockStatus::ORPHAN, {} };
	}

	if (pConfirmedChain->GetTipHash() == block.GetPreviousHash())
	{
		LOG_TRACE_F("Block {} is the next block. Processing it now.", block);
		return { EBlockStatus::NEXT_BLOCK, {} };
	}

	auto pForkBlock = std::make_shared<const FullBlock>(block);

	std::vector<FullBlock::CPtr> reorgBlocks({ pForkBlock });
	while (!pConfirmedChain->IsOnChain(pForkBlock->GetHeight() - 1, pForkBlock->GetPreviousHash()))
	{
		Hash previousHash = pForkBlock->GetPreviousHash();
		pForkBlock = pOrphanPool->GetOrphanBlock(pForkBlock->GetHeight() - 1, previousHash);
		if (pForkBlock == nullptr)
		{
			pForkBlock = pBlockDB->GetBlock(previousHash);
		}

		if (pForkBlock == nullptr)
		{
			LOG_TRACE_F("Mising previous block. Treating {} as orphan.", block);
			return { EBlockStatus::ORPHAN, {} };
		}

		reorgBlocks.push_back(pForkBlock);
	}

	std::reverse(reorgBlocks.begin(), reorgBlocks.end());

	if (pConfirmedChain->GetHeight() >= reorgBlocks.front()->GetHeight())
	{
		LOG_WARNING_F("Fork detected at height {}.", reorgBlocks.front()->GetHeight());
	}

	return { EBlockStatus::REORG, reorgBlocks };
}

void BlockProcessor::HandleReorg(Writer<ChainState> pBatch, const std::vector<FullBlock::CPtr>& reorgBlocks)
{
	const uint64_t totalDifficulty = pBatch->GetTotalDifficulty(EChainType::CONFIRMED);

	auto pTxHashSet = pBatch->GetTxHashSetManager()->GetTxHashSet();
	if (pTxHashSet == nullptr)
	{
		LOG_ERROR("TxHashSet is null. Still syncing?");
		throw BLOCK_CHAIN_EXCEPTION("TxHashSet is null");
	}

	auto pBlockDB = pBatch->GetBlockDB();
	auto pCommonHeader = pBlockDB->GetBlockHeader(reorgBlocks.front()->GetPreviousHash());
	if (pCommonHeader == nullptr)
	{
		LOG_ERROR_F("Failed to find header {}", reorgBlocks.front()->GetPreviousHash());
		throw BLOCK_CHAIN_EXCEPTION("Failed to find header.");
	}

	// TODO: Add rewound transactions back to TxPool

	pTxHashSet->Rewind(pBlockDB, *pCommonHeader);

	for (const FullBlock::CPtr& pBlock : reorgBlocks)
	{
		ValidateAndAddBlock(*pBlock, pBatch);
	}

	auto pConfirmedChain = pBatch->GetChainStore()->GetConfirmedChain();
	if (reorgBlocks.back()->GetTotalDifficulty() > totalDifficulty)
	{
		pConfirmedChain->Rewind(reorgBlocks.front()->GetHeight() - 1);
		for (const FullBlock::CPtr& pBlock : reorgBlocks)
		{
			pConfirmedChain->AddBlock(pBlock->GetHash());
		}

		pBatch->Commit();
	}
	else
	{
		// All blocks are valid, but since total difficulty did not increase, rollback all changes.
		// We will then re-add the blocks to the BlockDB so we know they've been validated.
		pBatch->Rollback();

		for (const FullBlock::CPtr& pBlock : reorgBlocks)
		{
			pBlockDB->AddBlock(*pBlock);
		}

		pBlockDB->Commit();
	}
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

	BlockSums blockSums = ContextualBlockValidator(m_config, pBlockDB, pTxHashSet).ValidateBlock(block);

	pBlockDB->RemoveOutputPositions(block.GetInputCommitments());
	pBlockDB->AddBlockSums(block.GetHash(), blockSums);
	pBlockDB->AddBlock(block);
	pOrphanPool->RemoveOrphan(block.GetHeight(), block.GetHash());
	pTxPool->ReconcileBlock(pBlockDB, pTxHashSet, block);
}