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

EBlockChainStatus BlockHeaderProcessor::ProcessSingleHeader(const BlockHeader& header)
{
	LOG_TRACE_F("Validating %s", header);

	auto pLockedState = m_pChainState->BatchWrite();

	auto pBlockDB = pLockedState->GetBlockDB();
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	auto pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	// Check if header already processed
	std::shared_ptr<const BlockIndex> pCandidateIndex = pCandidateChain->GetByHeight(header.GetHeight());
	if (pCandidateIndex != nullptr && pCandidateIndex->GetHash() == header.GetHash())
	{
		LOG_DEBUG_F("Header %s already processed.", header);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// If this is not the next header needed, process as an orphan.
	std::shared_ptr<const BlockIndex> pLastIndex = pCandidateChain->GetTip();
	if (pLastIndex->GetHash() != header.GetPreviousBlockHash())
	{
		return ProcessOrphan(pLockedState, header);
	}

	LOG_TRACE_F("Processing next candidate header: %s", header);

	// Validate the header.
	std::unique_ptr<BlockHeader> pPreviousHeaderPtr = pBlockDB->GetBlockHeader(pLastIndex->GetHash());
	if (!BlockHeaderValidator(m_config, pBlockDB, pHeaderMMR).IsValidHeader(header, *pPreviousHeaderPtr))
	{
		LOG_ERROR_F("Header %s failed to validate", header);
		throw BAD_DATA_EXCEPTION("Header failed to validate.");
	}

	pBlockDB->AddBlockHeader(header);
	pHeaderMMR->AddHeader(header);
	pSyncChain->AddBlock(header.GetHash());
	pCandidateChain->AddBlock(header.GetHash());

	pLockedState->Commit();

	LOG_DEBUG_F("Successfully validated %s", header);

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessOrphan(Writer<ChainState> pLockedState, const BlockHeader& header)
{
	auto pBlockDB = pLockedState->GetBlockDB();
	auto pOrphanPool = pLockedState->GetOrphanPool();
	auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	std::unique_ptr<BlockHeader> pCandidateHeader = pLockedState->GetTipBlockHeader(EChainType::CANDIDATE);
	if (header.GetTotalDifficulty() > pCandidateHeader->GetTotalDifficulty())
	{
		std::vector<std::shared_ptr<const BlockHeader>> reorgHeaders;
		std::shared_ptr<const BlockHeader> pHeader = std::make_shared<const BlockHeader>(header);
		while (pHeader != nullptr)
		{
			reorgHeaders.push_back(pHeader);
			std::shared_ptr<const BlockIndex> pIndex = pCandidateChain->GetByHeight(pHeader->GetHeight());
			if (pIndex != nullptr && pIndex->GetHash() == pHeader->GetHash())
			{
				// All headers exist. Reorg.
				std::reverse(reorgHeaders.begin(), reorgHeaders.end());

				const EBlockChainStatus processChunkStatus = ProcessChunkedSyncHeaders(pLockedState, reorgHeaders);
				if (processChunkStatus == EBlockChainStatus::SUCCESS || processChunkStatus == EBlockChainStatus::ALREADY_EXISTS)
				{
					pLockedState->Commit();
				}

				return processChunkStatus;
			}

			const Hash previousHash = pHeader->GetPreviousBlockHash();
			pHeader = pOrphanPool->GetOrphanHeader(previousHash);
			if (pHeader == nullptr)
			{
				pHeader = pBlockDB->GetBlockHeader(previousHash);
			}
		}
	}

	LOG_DEBUG_F("Processing header %s as an orphan.", header);
	pOrphanPool->AddOrphanHeader(header);
	return EBlockChainStatus::ORPHANED;
}

EBlockChainStatus BlockHeaderProcessor::ProcessSyncHeaders(const std::vector<BlockHeader>& headers)
{
	if (headers.empty())
	{
		return EBlockChainStatus::SUCCESS;
	}

	uint64_t height = headers.front().GetHeight();
	if (height == 0)
	{
		LOG_ERROR("Header with height 0 received.");
		throw BAD_DATA_EXCEPTION("Header with height 0 received.");
	}

	for (size_t i = 1; i < headers.size(); i++)
	{
		if (headers[i].GetHeight() != (headers[i - 1].GetHeight() + 1))
		{
			LOG_ERROR("Headers not sorted.");
			throw BAD_DATA_EXCEPTION("Headers not sorted.");
		}
	}

	const size_t size = headers.size();
	size_t index = 0;

	std::vector<std::shared_ptr<const BlockHeader>> chunkedHeaders;
	chunkedHeaders.reserve(SYNC_BATCH_SIZE);
	while (index < size)
	{
		chunkedHeaders.push_back(std::make_shared<BlockHeader>(headers[index++]));
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

EBlockChainStatus BlockHeaderProcessor::ProcessChunkedSyncHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers)
{
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	auto pChainStore = pLockedState->GetChainStore();
	auto pSyncChain = pChainStore->GetSyncChain();

	PrepareSyncChain(pLockedState, headers);

	// Filter out headers that are already part of sync chain.
	std::vector<std::shared_ptr<const BlockHeader>> newHeaders;
	for (size_t i = 0; i < headers.size(); i++)
	{
		std::shared_ptr<const BlockHeader> pHeader = headers[i];
		std::shared_ptr<const BlockIndex> pSyncHeader = pSyncChain->GetByHeight(pHeader->GetHeight());
		if (pSyncHeader == nullptr || pHeader->GetHash() != pSyncHeader->GetHash())
		{
			newHeaders.push_back(pHeader);
		}
	}

	if (newHeaders.empty())
	{
		LOG_DEBUG("Headers already processed.");
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	LOG_TRACE_F("Processing (%llu) headers.", newHeaders.size());

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

void BlockHeaderProcessor::PrepareSyncChain(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers)
{
	LOG_TRACE("Preparing sync chain");

	auto pChainStore = pLockedState->GetChainStore();
	auto pSyncChain = pChainStore->GetSyncChain();
	auto pCandidateChain = pChainStore->GetCandidateChain();

	// Check if previous header exists and matches previous hash.
	const Hash& previousHash = headers.front()->GetPreviousBlockHash();
	std::shared_ptr<const BlockIndex> pPrevSync = pSyncChain->GetByHeight(headers.front()->GetHeight() - 1);
	if (pPrevSync == nullptr || pPrevSync->GetHash() != previousHash)
	{
		auto pPrevCandidate = pCandidateChain->GetByHeight(headers.front()->GetHeight() - 1);
		if (pPrevCandidate != nullptr && pPrevCandidate->GetHash() == previousHash)
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

void BlockHeaderProcessor::RewindMMR(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers)
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

void BlockHeaderProcessor::ValidateHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers)
{
	LOG_TRACE("Validating headers");

	auto pBlockDB = pLockedState->GetBlockDB();
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	BlockHeaderValidator validator(m_config, pBlockDB, pHeaderMMR);

	const Hash& previousHash = headers.front()->GetPreviousBlockHash();
	std::shared_ptr<const BlockHeader> pPreviousHeader = pBlockDB->GetBlockHeader(previousHash);
	if (pPreviousHeader == nullptr)
	{
		LOG_ERROR_F("Previous header (%s) not found.", previousHash);
		throw BLOCK_CHAIN_EXCEPTION("Failed to retrieve previous header");
	}

	for (std::shared_ptr<const BlockHeader> pHeader : headers)
	{
		if (!validator.IsValidHeader(*pHeader, *pPreviousHeader))
		{
			LOG_ERROR_F("Header invalid: %s", *pHeader);
			throw BAD_DATA_EXCEPTION("Header invalid.");
		}

		pHeaderMMR->AddHeader(*pHeader);
		pBlockDB->AddBlockHeader(*pHeader);
		pPreviousHeader = pHeader;
	}
}

void BlockHeaderProcessor::AddSyncHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers)
{
	LOG_TRACE("Applying headers to sync chain");

	auto pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	const uint64_t firstHeaderHeight = headers.front()->GetHeight();
	const Hash& previousHash = headers.front()->GetPreviousBlockHash();

	// Ensure chain is on correct fork.
	auto pPrevious = pSyncChain->GetByHeight(firstHeaderHeight - 1);
	if (pPrevious == nullptr || pPrevious->GetHash() != previousHash)
	{
		LOG_ERROR_F("Chain state invalid. Unrecoverable error.");
		throw BLOCK_CHAIN_EXCEPTION("Chain state invalid.");
	}

	// Rewind chain if necessary.
	if (pSyncChain->GetTip()->GetHash() != previousHash)
	{
		pSyncChain->Rewind(firstHeaderHeight - 1);
	}

	for (std::shared_ptr<const BlockHeader> pHeader : headers)
	{
		pSyncChain->AddBlock(pHeader->GetHash());
	}
}