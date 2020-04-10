#include "BlockHeaderProcessor.h"
#include "../Validators/BlockHeaderValidator.h"

#include <Core/Exceptions/BadDataException.h>
#include <Core/Exceptions/BlockChainException.h>
#include <Infrastructure/Logger.h>
#include <PMMR/HeaderMMR.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>

static const size_t SYNC_BATCH_SIZE = 128;

BlockHeaderProcessor::BlockHeaderProcessor(const Config& config, std::shared_ptr<Locked<ChainState>> pChainState)
	: m_config(config), m_pChainState(pChainState)
{

}

EBlockChainStatus BlockHeaderProcessor::ProcessSingleHeader(const BlockHeaderPtr& pHeader)
{
	LOG_TRACE_F("Validating {}", *pHeader);

	auto pLockedState = m_pChainState->BatchWrite();

	EBlockChainStatus status = ProcessSingleHeader(pHeader, pLockedState);
	if (status == EBlockChainStatus::SUCCESS || status == EBlockChainStatus::ALREADY_EXISTS)
	{
		LOG_DEBUG_F("Successfully validated {}", *pHeader);
		pLockedState->Commit();
	}

	return status;
}

EBlockChainStatus BlockHeaderProcessor::ProcessSingleHeader(const BlockHeaderPtr& pHeader, Writer<ChainState>& pLockedState)
{
	auto pBlockDB = pLockedState->GetBlockDB();
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	auto pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	// Check if header already processed
	if (pBlockDB->GetBlockHeader(pHeader->GetHash()) != nullptr)
	{
		LOG_TRACE_F("Header {} already processed.", *pHeader);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// If this is not the next header needed, process as an orphan.
	if (pCandidateChain->GetTipHash() != pHeader->GetPreviousHash())
	{
		return ProcessOrphan(pLockedState, pHeader);
	}

	LOG_TRACE_F("Processing next candidate header: {}", *pHeader);

	// Validate the header.
	auto pPreviousHeaderPtr = pBlockDB->GetBlockHeader(pCandidateChain->GetTipHash());
	if (!BlockHeaderValidator(m_config, pBlockDB, pHeaderMMR).IsValidHeader(*pHeader, *pPreviousHeaderPtr))
	{
		LOG_ERROR_F("Header {} failed to validate", *pHeader);
		throw BAD_DATA_EXCEPTION("Header failed to validate.");
	}

	pBlockDB->AddBlockHeader(pHeader);
	pHeaderMMR->AddHeader(*pHeader);
	pSyncChain->AddBlock(pHeader->GetHash());
	pCandidateChain->AddBlock(pHeader->GetHash());

	LOG_DEBUG_F("Successfully validated {}", *pHeader);

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessOrphan(Writer<ChainState> pLockedState, BlockHeaderPtr pHeader)
{
	auto pBlockDB = pLockedState->GetBlockDB();
	auto pOrphanPool = pLockedState->GetOrphanPool();
	auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();
	auto pHeaderMMR = pLockedState->GetHeaderMMR();

	const uint64_t totalDifficulty = pLockedState->GetTotalDifficulty(EChainType::CANDIDATE);

	std::vector<BlockHeaderPtr> reorgHeaders;
	auto pTempHeader = pHeader;
	while (pTempHeader != nullptr)
	{
		if (pCandidateChain->IsOnChain(pTempHeader))
		{
			// All headers exist. Reorg.
			std::reverse(reorgHeaders.begin(), reorgHeaders.end());
			break;
		}

		reorgHeaders.push_back(pTempHeader);
		const Hash& previousHash = pTempHeader->GetPreviousHash();
		pTempHeader = pOrphanPool->GetOrphanHeader(previousHash);
		if (pTempHeader == nullptr)
		{
			pTempHeader = pBlockDB->GetBlockHeader(previousHash);
		}
	}

	if (pTempHeader == nullptr)
	{
		LOG_DEBUG_F("Processing header {} as an orphan.", *pHeader);
		pOrphanPool->AddOrphanHeader(pHeader);
		return EBlockChainStatus::ORPHANED;
	}

	// Rewind to fork point
	pHeaderMMR->Rewind(reorgHeaders.front()->GetHeight());

	// Validate each header and add it to the MMR & BlockDB
	ValidateHeaders(pLockedState, reorgHeaders);

	if (pHeader->GetTotalDifficulty() <= totalDifficulty)
	{
		// The header MMR should always match the candidate chain.
		// Since the total difficulty was not enough to reorg the candidate chain, we must rollback our MMR changes.
		LOG_INFO("Total difficulty did not increase. Rolling back header MMR changes.");
		pHeaderMMR->Rollback();
	}
	else
	{
		pCandidateChain->Rewind(reorgHeaders.front()->GetHeight() - 1);

		for (const BlockHeaderPtr& pHeaders : reorgHeaders)
		{
			pCandidateChain->AddBlock(pHeaders->GetHash());
		}
	}

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessSyncHeaders(const std::vector<BlockHeaderPtr>& headers)
{
	if (headers.empty())
	{
		return EBlockChainStatus::SUCCESS;
	}

	uint64_t height = headers.front()->GetHeight();
	if (height == 0)
	{
		LOG_ERROR("Header with height 0 received.");
		throw BAD_DATA_EXCEPTION("Header with height 0 received.");
	}

	for (size_t i = 1; i < headers.size(); i++)
	{
		if (headers[i]->GetHeight() != (headers[i - 1]->GetHeight() + 1))
		{
			LOG_ERROR("Headers not sorted.");
			throw BAD_DATA_EXCEPTION("Headers not sorted.");
		}
	}

	const size_t size = headers.size();
	size_t index = 0;

	std::vector<BlockHeaderPtr> chunkedHeaders;
	chunkedHeaders.reserve(SYNC_BATCH_SIZE);
	while (index < size)
	{
		chunkedHeaders.push_back(headers[index++]);
		if (index % SYNC_BATCH_SIZE == 0 || index == size)
		{
			auto pChainStateBatch = m_pChainState->BatchWrite();
			const EBlockChainStatus processChunkStatus = ProcessChunkedSyncHeaders(pChainStateBatch, chunkedHeaders);
			if (processChunkStatus == EBlockChainStatus::SUCCESS)
			{
				pChainStateBatch->Commit();
			}
			else if (processChunkStatus != EBlockChainStatus::ALREADY_EXISTS)
			{
				return processChunkStatus;
			}

			chunkedHeaders.clear();
		}
	}

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessChunkedSyncHeaders(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	auto pChainStore = pLockedState->GetChainStore();
	auto pSyncChain = pChainStore->GetSyncChain();

	PrepareSyncChain(pLockedState, headers);

	// Filter out headers that are already part of sync chain.
	std::vector<BlockHeaderPtr> newHeaders;
	for (size_t i = 0; i < headers.size(); i++)
	{
		auto pHeader = headers[i];
		if (!pSyncChain->IsOnChain(pHeader))
		{
			newHeaders.push_back(pHeader);
		}
	}

	if (newHeaders.empty())
	{
		LOG_DEBUG("Headers already processed.");
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	LOG_TRACE_F("Processing {} headers.", newHeaders.size());

	// Rewind MMR
	RewindMMR(pLockedState, newHeaders);

	// Validate the headers.
	ValidateHeaders(pLockedState, newHeaders);

	// Add the headers to the sync chain.
	AddSyncHeaders(pLockedState, newHeaders);

	// If total difficulty increases, accept sync chain as new candidate chain.
	if (pLockedState->GetTotalDifficulty(EChainType::SYNC) > pLockedState->GetTotalDifficulty(EChainType::CANDIDATE))
	{
		LOG_DEBUG("Total difficulty increased. Applying headers to candidate chain.");
		pChainStore->ReorgChain(EChainType::SYNC, EChainType::CANDIDATE);
	}
	else
	{
		// The header MMR should always match the candidate chain.
		// Since the total difficulty was not enough to reorg the candidate chain, we must rollback our MMR changes.
		LOG_INFO("Total difficulty did not increase. Rolling back header MMR changes.");
		pHeaderMMR->Rollback();
	}

	return EBlockChainStatus::SUCCESS;
}

void BlockHeaderProcessor::PrepareSyncChain(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
	LOG_TRACE("Preparing sync chain");

	auto pChainStore = pLockedState->GetChainStore();
	auto pSyncChain = pChainStore->GetSyncChain();
	auto pCandidateChain = pChainStore->GetCandidateChain();

	// Check if previous header exists and matches previous hash.
	const uint64_t previousHeight = headers.front()->GetHeight() - 1;
	const Hash& previousHash = headers.front()->GetPreviousHash();
	if (!pSyncChain->IsOnChain(previousHeight, previousHash))
	{
		if (pCandidateChain->IsOnChain(previousHeight, previousHash))
		{
			pChainStore->ReorgChain(EChainType::CANDIDATE, EChainType::SYNC);
		}
		else
		{
			LOG_INFO("Previous header doesn't match. Still syncing?");
			throw BLOCK_CHAIN_EXCEPTION("Previous header not found.");
		}
	}
}

void BlockHeaderProcessor::RewindMMR(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
	LOG_TRACE("Rewinding MMR");

	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	auto pChainStore = pLockedState->GetChainStore();

	const uint64_t firstHeight = headers.front()->GetHeight();

	auto pCommonIndex = pChainStore->FindCommonIndex(EChainType::SYNC, EChainType::CANDIDATE);
	if (pCommonIndex->GetHeight() < (firstHeight - 1))
	{
		pHeaderMMR->Rewind(pCommonIndex->GetHeight() + 1);
		for (size_t height = pCommonIndex->GetHeight() + 1; height < firstHeight; height++)
		{
			auto pHeader = pLockedState->GetBlockHeaderByHeight(height, EChainType::SYNC);
			if (pHeader == nullptr)
			{
				throw BLOCK_CHAIN_EXCEPTION("Failed to retrieve header");
			}

			pHeaderMMR->AddHeader(*pHeader);
		}
	}
	else
	{
		pHeaderMMR->Rewind(firstHeight);
	}
}

void BlockHeaderProcessor::ValidateHeaders(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
	LOG_TRACE("Validating headers");

	auto pBlockDB = pLockedState->GetBlockDB();
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	BlockHeaderValidator validator(m_config, pBlockDB, pHeaderMMR);

	const Hash& previousHash = headers.front()->GetPreviousHash();
	auto pPreviousHeader = pBlockDB->GetBlockHeader(previousHash);
	if (pPreviousHeader == nullptr)
	{
		LOG_ERROR_F("Previous header ({}) not found.", previousHash);
		throw BLOCK_CHAIN_EXCEPTION("Failed to retrieve previous header");
	}

	for (auto pHeader : headers)
	{
		if (!validator.IsValidHeader(*pHeader, *pPreviousHeader))
		{
			LOG_ERROR_F("Header invalid: {}", *pHeader);
			throw BAD_DATA_EXCEPTION("Header invalid.");
		}

		pHeaderMMR->AddHeader(*pHeader);
		pBlockDB->AddBlockHeader(pHeader);
		pPreviousHeader = pHeader;
	}
}

void BlockHeaderProcessor::AddSyncHeaders(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
	LOG_TRACE("Applying headers to sync chain");

	auto pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	const uint64_t firstHeaderHeight = headers.front()->GetHeight();
	const Hash& previousHash = headers.front()->GetPreviousHash();

	// Ensure chain is on correct fork.
	if (!pSyncChain->IsOnChain(firstHeaderHeight - 1, previousHash))
	{
		LOG_ERROR("Chain state invalid. Unrecoverable error.");
		throw BLOCK_CHAIN_EXCEPTION("Chain state invalid.");
	}

	// Rewind chain if necessary.
	if (pSyncChain->GetTipHash() != previousHash)
	{
		pSyncChain->Rewind(firstHeaderHeight - 1);
	}

	for (auto pHeader : headers)
	{
		pSyncChain->AddBlock(pHeader->GetHash());
	}
}