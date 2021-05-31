#include "BlockProcessor.h"
#include "BlockHeaderProcessor.h"
#include "../Validators/BlockValidator.h"

#include <Consensus.h>
#include <Core/Exceptions/BlockChainException.h>
#include <Core/Exceptions/BadDataException.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Common/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <algorithm>

BlockProcessor::BlockProcessor(const std::shared_ptr<Locked<ChainState>>& pChainState)
	: m_pChainState(pChainState) { }

EBlockChainStatus BlockProcessor::ProcessBlock(const FullBlock& block)
{	
	const uint64_t candidateHeight = m_pChainState->Read()->GetHeight(EChainType::CANDIDATE);
	const uint64_t horizonHeight = Consensus::GetHorizonHeight(candidateHeight);

	BlockHeaderPtr pHeader = block.GetHeader();
	if (pHeader->GetHeight() <= horizonHeight) {
		throw BAD_DATA_EXCEPTION(EBanReason::BadBlock, "Can't process blocks beyond horizon.");
	}

	// Make sure header is processed and valid before processing block.
	BlockHeaderProcessor(m_pChainState).ProcessSingleHeader(pHeader);

	// Verify block is self-consistent before locking
	BlockValidator::VerifySelfConsistent(block);

	const EBlockChainStatus returnStatus = ProcessBlockInternal(block);
	if (returnStatus == EBlockChainStatus::SUCCESS) {
		LOG_DEBUG_F("Block {} successfully processed.", *pHeader);
	}

	return returnStatus;
}

EBlockChainStatus BlockProcessor::ProcessBlockInternal(const FullBlock& block)
{
	auto pBatch = m_pChainState->BatchWrite();
	auto pChainStore = pBatch->GetChainStore();
	auto pOrphanPool = pBatch->GetOrphanPool();
	auto pConfirmedChain = pChainStore->GetConfirmedChain();

	// 1. Check if already part of confirmed chain
	if (pConfirmedChain->IsOnChain(block.GetHeader())) {
		LOG_TRACE_F("Block {} already part of confirmed chain.", block);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 2. Check if block has already been processed.
	if (pBatch->GetBlockDB()->GetBlock(block.GetHash()) != nullptr) {
		LOG_TRACE_F("Block {} already processed.", block);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 3. Orphan if block should be processed as an orphan
	const BlockProcessingInfo info = DetermineBlockStatus(block, pBatch);
	if (info.status == EBlockStatus::ORPHAN) {
		if (pOrphanPool->IsOrphan(block.GetHeight(), block.GetHash())) {
			LOG_TRACE_F("Block {} already processed as an orphan.", block);
			return EBlockChainStatus::ALREADY_EXISTS;
		}

		pOrphanPool->AddOrphanBlock(block);

		return EBlockChainStatus::ORPHANED;
	}

	if (info.status == EBlockStatus::REORG) {
		HandleReorg(pBatch, info.reorgBlocks);
	} else {
		assert(info.status == EBlockStatus::NEXT_BLOCK);

		ValidateAndAddBlock(block, pBatch);
		pConfirmedChain->AddBlock(block.GetHash(), block.GetHeight());
		pBatch->Commit();
	}

	return EBlockChainStatus::SUCCESS;
}

BlockProcessor::BlockProcessingInfo BlockProcessor::DetermineBlockStatus(const FullBlock& block, Writer<ChainState> pBatch)
{
	auto pOrphanPool = pBatch->GetOrphanPool();
	auto pBlockDB = pBatch->GetBlockDB();
	auto pChainStore = pBatch->GetChainStore();
	auto pConfirmedChain = pChainStore->GetConfirmedChain();

	auto pStoreHeader = pBlockDB->GetBlockHeader(block.GetHash());
	if (pStoreHeader == nullptr) {
		LOG_TRACE_F("Header has not yet been validated. Treating {} as orphan.", block);
		return { EBlockStatus::ORPHAN, {} };
	}

	if (pConfirmedChain->GetTipHash() == block.GetPreviousHash()) {
		LOG_TRACE_F("Block {} is the next block. Processing it now.", block);
		return { EBlockStatus::NEXT_BLOCK, {} };
	}

	auto pForkBlock = std::make_shared<const FullBlock>(block);

	std::vector<FullBlock::CPtr> reorgBlocks({ pForkBlock });
	while (!pConfirmedChain->IsOnChain(pForkBlock->GetHeight() - 1, pForkBlock->GetPreviousHash()))
	{
		Hash previousHash = pForkBlock->GetPreviousHash();
		pForkBlock = pOrphanPool->GetOrphanBlock(pForkBlock->GetHeight() - 1, previousHash);
		if (pForkBlock == nullptr) {
			pForkBlock = pBlockDB->GetBlock(previousHash);
		}

		if (pForkBlock == nullptr) {
			LOG_TRACE_F("Missing previous block. Treating {} as orphan.", block);
			return { EBlockStatus::ORPHAN, {} };
		}

		reorgBlocks.push_back(pForkBlock);
	}

	std::reverse(reorgBlocks.begin(), reorgBlocks.end());

	if (pConfirmedChain->GetHeight() >= reorgBlocks.front()->GetHeight()) {
		LOG_WARNING_F("Fork detected at height {}.", reorgBlocks.front()->GetHeight());
	}

	return { EBlockStatus::REORG, reorgBlocks };
}

void BlockProcessor::HandleReorg(Writer<ChainState> pBatch, const std::vector<FullBlock::CPtr>& reorgBlocks)
{
	const uint64_t totalDifficulty = pBatch->GetTotalDifficulty(EChainType::CONFIRMED);

	auto pTxHashSet = pBatch->GetTxHashSetManager()->GetTxHashSet();
	if (pTxHashSet == nullptr) {
		LOG_ERROR("TxHashSet is null. Still syncing?");
		throw BLOCK_CHAIN_EXCEPTION("TxHashSet is null");
	}

	auto pBlockDB = pBatch->GetBlockDB();
	auto pCommonHeader = pBlockDB->GetBlockHeader(reorgBlocks.front()->GetPreviousHash());
	if (pCommonHeader == nullptr) {
		LOG_ERROR_F("Failed to find header {}", reorgBlocks.front()->GetPreviousHash());
		throw BLOCK_CHAIN_EXCEPTION("Failed to find header.");
	}

	// TODO: Add rewound transactions back to TxPool

	pTxHashSet->Rewind(pBlockDB, *pCommonHeader);

	for (const FullBlock::CPtr& pBlock : reorgBlocks)
	{
		ValidateAndAddBlock(*pBlock, pBatch);
	}

	if (reorgBlocks.back()->GetTotalDifficulty() > totalDifficulty)
	{
		auto pConfirmedChain = pBatch->GetChainStore()->GetConfirmedChain();
		pConfirmedChain->Rewind(reorgBlocks.front()->GetHeight() - 1);
		for (const FullBlock::CPtr& pBlock : reorgBlocks)
		{
			pConfirmedChain->AddBlock(pBlock->GetHash(), pBlock->GetHeight());
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
	if (pPreviousHeader == nullptr) {
		LOG_ERROR_F("Failed to find header {}", previousHash);
		throw BLOCK_CHAIN_EXCEPTION("Previous header not found.");
	}

	if (pTxHashSet == nullptr || !pTxHashSet->ApplyBlock(pBlockDB, block)) {
		throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlock, "Failed to apply block {} to the TxHashSet.", block);
	}

	BlockValidator::VerifySelfConsistent(block);

	if (!pTxHashSet->ValidateRoots(*block.GetHeader())) {
		throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlock, "Failed to validate TxHashSet roots for block {}.", block);
	}

	std::unique_ptr<BlockSums> pPreviousBlockSums = pBlockDB->GetBlockSums(previousHash);
	if (pPreviousBlockSums == nullptr) {
		LOG_WARNING_F("Failed to retrieve block sums for block {}", previousHash);
		throw BLOCK_CHAIN_EXCEPTION("Failed to retrieve block sums.");
	}

	BlockSums blockSums = KernelSumValidator::ValidateKernelSums(
		block.GetTransactionBody(),
		0 - Consensus::REWARD,
		block.GetTotalKernelOffset(),
		std::make_optional(*pPreviousBlockSums)
	);

	pBlockDB->AddBlockSums(block.GetHash(), blockSums);
	pBlockDB->AddBlock(block);
	pOrphanPool->RemoveOrphan(block.GetHeight(), block.GetHash());
	pTxPool->ReconcileBlock(pBlockDB, pTxHashSet, block);
}