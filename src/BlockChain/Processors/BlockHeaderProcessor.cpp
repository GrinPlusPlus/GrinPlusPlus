#include "BlockHeaderProcessor.h"
#include "../Validators/BlockHeaderValidator.h"

#include <Core/Exceptions/BadDataException.h>
#include <Core/Exceptions/BlockChainException.h>
#include <Infrastructure/Logger.h>
#include <PMMR/HeaderMMR.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>

static const size_t SYNC_BATCH_SIZE = 32;

BlockHeaderProcessor::BlockHeaderProcessor(const Config& config, std::shared_ptr<Locked<ChainState>> pChainState)
	: m_config(config), m_pChainState(pChainState)
{

}

EBlockChainStatus BlockHeaderProcessor::ProcessSingleHeader(const BlockHeader& header)
{
	LOG_TRACE_F("Validating %s", header);

	auto pLockedState = m_pChainState->BatchWrite();
	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	// Check if header already processed
	std::shared_ptr<const BlockIndex> pCandidateIndex = pCandidateChain->GetByHeight(header.GetHeight());
	if (pCandidateIndex != nullptr && pCandidateIndex->GetHash() == header.GetHash())
	{
		LOG_TRACE_F("Header %s already processed.", header);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// If this is not the next header needed, process as an orphan.
	std::shared_ptr<const BlockIndex> pLastIndex = pCandidateChain->GetTip();
	if (pLastIndex->GetHash() != header.GetPreviousBlockHash())
	{
		std::unique_ptr<BlockHeader> pCandidateHeader = pLockedState->GetBlockDB()->GetBlockHeader(pLastIndex->GetHash());
		if (header.GetTotalDifficulty() > pCandidateHeader->GetTotalDifficulty())
		{
			std::vector<std::shared_ptr<const BlockHeader>> reorgHeaders;
			std::shared_ptr<BlockHeader> pHeader = std::make_shared<BlockHeader>(BlockHeader(header));
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
				pHeader = pLockedState->GetOrphanPool()->GetOrphanHeader(previousHash);
				if (pHeader == nullptr)
				{
					pHeader = pLockedState->GetBlockDB()->GetBlockHeader(previousHash);
				}
			}
		}

		LOG_DEBUG_F("Processing header %s as an orphan.", header);
		pLockedState->GetOrphanPool()->AddOrphanHeader(header);
		return EBlockChainStatus::ORPHANED;
	}

	LOG_TRACE_F("Processing next candidate header: %s", header);

	// Validate the header.
	std::unique_ptr<BlockHeader> pPreviousHeaderPtr = pLockedState->GetBlockDB()->GetBlockHeader(pLastIndex->GetHash());
	if (!BlockHeaderValidator(m_config, pLockedState->GetBlockDB(), pLockedState->GetHeaderMMR()).IsValidHeader(header, *pPreviousHeaderPtr))
	{
		LOG_ERROR_F("Header %s failed to validate", header);
		throw BAD_DATA_EXCEPTION("Header failed to validate.");
	}

	pLockedState->GetBlockDB()->AddBlockHeader(header);
	pLockedState->GetHeaderMMR()->AddHeader(header);

	pLockedState->GetChainStore()->GetSyncChain()->AddBlock(header.GetHash());
	pCandidateChain->AddBlock(header.GetHash());
	pLockedState->Commit();

	LOG_DEBUG_F("Successfully validated %s", header);

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessSyncHeaders(const std::vector<BlockHeader>& headers)
{
	if (headers.empty())
	{
		return EBlockChainStatus::SUCCESS;
	}

	const uint64_t height = headers.front().GetHeight();
	if (height == 0)
	{
		LOG_ERROR("Header with height 0 received.");
		throw BAD_DATA_EXCEPTION("Header with height 0 received.");
	}

	const size_t size = headers.size();
	size_t index = 0;

	std::vector<std::shared_ptr<const BlockHeader>> chunkedHeaders;
	chunkedHeaders.reserve(SYNC_BATCH_SIZE);
	while (index < size)
	{
		chunkedHeaders.push_back(std::make_shared<BlockHeader>(BlockHeader(headers[index++])));
		if (index % SYNC_BATCH_SIZE == 0)
		{
			auto pChainStateBatch = m_pChainState->BatchWrite();
			const EBlockChainStatus processChunkStatus = ProcessChunkedSyncHeaders(pChainStateBatch, chunkedHeaders);
			if (processChunkStatus != EBlockChainStatus::SUCCESS && processChunkStatus != EBlockChainStatus::ALREADY_EXISTS)
			{
				return processChunkStatus;
			}

			pChainStateBatch->Commit();
			chunkedHeaders.clear();
		}
	}

	if (!chunkedHeaders.empty())
	{
		auto pChainStateBatch = m_pChainState->BatchWrite();
		const EBlockChainStatus processChunkStatus = ProcessChunkedSyncHeaders(pChainStateBatch, chunkedHeaders);
		if (processChunkStatus == EBlockChainStatus::SUCCESS)
		{
			pChainStateBatch->Commit();
		}
		
		return processChunkStatus;
	}

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessChunkedSyncHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers)
{
	std::shared_ptr<Chain> pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

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
		LOG_TRACE("Headers already processed.");
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	LOG_TRACE_F("Processing (%llu) headers.", newHeaders.size());

	// Check if previous header exists and matches previous.
	std::shared_ptr<const BlockIndex> pPrevIndex = pSyncChain->GetByHeight(newHeaders.front()->GetHeight() - 1);
	if (pPrevIndex == nullptr || pPrevIndex->GetHash() != newHeaders.front()->GetPreviousBlockHash())
	{
		LOG_INFO("Previous header doesn't match. Still syncing?");
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	// Rewind MMR
	std::shared_ptr<IHeaderMMR> pHeaderMMR = pLockedState->GetHeaderMMR();
	pHeaderMMR->Rewind(newHeaders.front()->GetHeight());

	// Validate the headers.
	ValidateHeaders(pLockedState, newHeaders);

	// Add the headers to the chain state.
	AddSyncHeaders(pLockedState, newHeaders);

	// If total difficulty increases, accept sync chain as new candidate chain.
	if (CheckAndAcceptSyncChain(pLockedState))
	{
		pLockedState->Commit();
	}
	else
	{
		pLockedState->Rollback();
	}

	return EBlockChainStatus::SUCCESS;
}

void BlockHeaderProcessor::ValidateHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers)
{
	auto pBlockDB = pLockedState->GetBlockDB();
	std::shared_ptr<const BlockHeader> pPreviousHeader = pBlockDB->GetBlockHeader(headers.front()->GetPreviousBlockHash());
	for (std::shared_ptr<const BlockHeader> pHeader : headers)
	{
		if (!BlockHeaderValidator(m_config, pBlockDB, pLockedState->GetHeaderMMR()).IsValidHeader(*pHeader, *pPreviousHeader))
		{
			LOG_ERROR_F("Header invalid: %s", *pHeader);
			throw BAD_DATA_EXCEPTION("Header invalid.");
		}

		pLockedState->GetHeaderMMR()->AddHeader(*pHeader);
		pBlockDB->AddBlockHeader(*pHeader);
		pPreviousHeader = pHeader;
	}
}

void BlockHeaderProcessor::AddSyncHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers) const
{
	std::shared_ptr<Chain> pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	const uint64_t firstHeaderHeight = (*headers.begin())->GetHeight();

	// Ensure chain is on correct fork.
	std::shared_ptr<const BlockIndex> pPrevious = pSyncChain->GetByHeight(firstHeaderHeight - 1);
	if (pPrevious == nullptr || pPrevious->GetHash() != (*headers.begin())->GetPreviousBlockHash())
	{
		LOG_ERROR_F("Chain state invalid. Unrecoverable error.");
		throw BLOCK_CHAIN_EXCEPTION("Chain state invalid.");
	}

	// Rewind chain if necessary.
	if (pSyncChain->GetTip()->GetHash() != (*headers.begin())->GetPreviousBlockHash())
	{
		pSyncChain->Rewind(firstHeaderHeight - 1);
	}

	for (std::shared_ptr<const BlockHeader> pHeader : headers)
	{
		pSyncChain->AddBlock(pHeader->GetHash());
	}
}

bool BlockHeaderProcessor::CheckAndAcceptSyncChain(Writer<ChainState> pLockedState) const
{
	std::unique_ptr<BlockHeader> pSyncHead = pLockedState->GetTipBlockHeader(EChainType::SYNC);
	std::unique_ptr<BlockHeader> pCandidateHead = pLockedState->GetTipBlockHeader(EChainType::CANDIDATE);
	if (pSyncHead == nullptr || pCandidateHead == nullptr)
	{
		return false;
	}

	if (pSyncHead->GetTotalDifficulty() > pCandidateHead->GetTotalDifficulty())
	{
		return pLockedState->GetChainStore()->ReorgChain(EChainType::SYNC, EChainType::CANDIDATE, pSyncHead->GetHeight());
	}

	return false;
}