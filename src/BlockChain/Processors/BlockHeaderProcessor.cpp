#include "BlockHeaderProcessor.h"
#include "../Validators/BlockHeaderValidator.h"

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
	BlockIndex* pCandidateIndex = pCandidateChain->GetByHeight(header.GetHeight());
	if (pCandidateIndex != nullptr && pCandidateIndex->GetHash() == header.GetHash())
	{
		LOG_TRACE_F("Header %s already processed.", header);
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// If this is not the next header needed, process as an orphan.
	BlockIndex* pLastIndex = pCandidateChain->GetTip();
	if (pLastIndex->GetHash() != header.GetPreviousBlockHash())
	{
		std::unique_ptr<BlockHeader> pCandidateHeader = pLockedState->GetBlockDB()->GetBlockHeader(pLastIndex->GetHash());
		if (header.GetTotalDifficulty() > pCandidateHeader->GetTotalDifficulty())
		{
			std::vector<BlockHeader> headers;
			std::unique_ptr<BlockHeader> pHeader = std::make_unique<BlockHeader>(BlockHeader(header));
			while (pHeader != nullptr)
			{
				headers.push_back(*pHeader);
				BlockIndex* pIndex = pCandidateChain->GetByHeight(pHeader->GetHeight());
				if (pIndex != nullptr && pIndex->GetHash() == pHeader->GetHash())
				{
					// All headers exist. Reorg.
					std::vector<const BlockHeader*> reorgHeaders;
					for (auto iter = headers.rbegin(); iter != headers.rend(); iter++)
					{
						const BlockHeader& reorgHeader = *iter;
						reorgHeaders.push_back(&reorgHeader);
					}

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
		return EBlockChainStatus::INVALID;
	}

	pLockedState->GetBlockDB()->AddBlockHeader(header);
	pLockedState->GetHeaderMMR()->AddHeader(header);
	//lockedState.m_headerMMR.Commit();

	BlockIndex* pBlockIndex = pLockedState->GetChainStore()->GetOrCreateIndex(header.GetHash(), header.GetHeight());
	pLockedState->GetChainStore()->GetSyncChain()->AddBlock(pBlockIndex);
	pCandidateChain->AddBlock(pBlockIndex);
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
		return EBlockChainStatus::INVALID;
	}

	const size_t size = headers.size();
	size_t index = 0;

	std::vector<const BlockHeader*> chunkedHeaders;
	chunkedHeaders.reserve(SYNC_BATCH_SIZE);
	while (index < size)
	{
		chunkedHeaders.push_back(&headers[index++]);
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

EBlockChainStatus BlockHeaderProcessor::ProcessChunkedSyncHeaders(Writer<ChainState> pLockedState, const std::vector<const BlockHeader*>& headers)
{
	std::shared_ptr<Chain> pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	// Filter out headers that are already part of sync chain.
	std::vector<const BlockHeader*> newHeaders;
	for (size_t i = 0; i < headers.size(); i++)
	{
		const BlockHeader* pHeader = headers[i];
		BlockIndex* pSyncHeader = pSyncChain->GetByHeight(pHeader->GetHeight());
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
	BlockIndex* pPrevIndex = pSyncChain->GetByHeight(newHeaders.front()->GetHeight() - 1);
	if (pPrevIndex == nullptr || pPrevIndex->GetHash() != newHeaders.front()->GetPreviousBlockHash())
	{
		LOG_INFO("Previous header doesn't match. Still syncing?");
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	// Rewind MMR
	std::shared_ptr<IHeaderMMR> pHeaderMMR = pLockedState->GetHeaderMMR();
	pHeaderMMR->Rewind(newHeaders.front()->GetHeight());

	// Validate the headers.
	EBlockChainStatus validateStatus = ValidateHeaders(pLockedState, newHeaders);
	if (validateStatus != EBlockChainStatus::SUCCESS)
	{
		pLockedState->Rollback();
		return validateStatus;
	}

	// Add the headers to the chain state.
	const EBlockChainStatus addSyncHeadersStatus = AddSyncHeaders(pLockedState, newHeaders);
	if (addSyncHeadersStatus != EBlockChainStatus::SUCCESS)
	{
		LOG_ERROR("Failed to add sync headers.");
		pLockedState->GetHeaderMMR()->Rollback();
		pLockedState->GetChainStore()->ReorgChain(EChainType::CANDIDATE, EChainType::SYNC, pCandidateChain->GetTip()->GetHeight());
		// TODO: pLockedState->Rollback()? Commit?

		return addSyncHeadersStatus;
	}

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

EBlockChainStatus BlockHeaderProcessor::ValidateHeaders(Writer<ChainState> pLockedState, const std::vector<const BlockHeader*>& headers)
{
	auto pBlockDB = pLockedState->GetBlockDB();
	std::unique_ptr<BlockHeader> pPreviousHeaderPtr = pBlockDB->GetBlockHeader(headers.front()->GetPreviousBlockHash());
	const BlockHeader* pPreviousHeader = pPreviousHeaderPtr.get();
	for (const BlockHeader* pHeader : headers)
	{
		if (!BlockHeaderValidator(m_config, pBlockDB, pLockedState->GetHeaderMMR()).IsValidHeader(*pHeader, *pPreviousHeader))
		{
			return EBlockChainStatus::INVALID;
		}

		pLockedState->GetHeaderMMR()->AddHeader(*pHeader);
		pBlockDB->AddBlockHeader(*pHeader);
		pPreviousHeader = pHeader;
	}

	//pDatabase->Commit();
	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::AddSyncHeaders(Writer<ChainState> pLockedState, const std::vector<const BlockHeader*>& headers) const
{
	std::shared_ptr<Chain> pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	const uint64_t firstHeaderHeight = (*headers.begin())->GetHeight();

	// Ensure chain is on correct fork.
	BlockIndex* pPrevious = pSyncChain->GetByHeight(firstHeaderHeight - 1);
	if (pPrevious == nullptr || pPrevious->GetHash() != (*headers.begin())->GetPreviousBlockHash())
	{
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	// Rewind chain if necessary.
	if (pSyncChain->GetTip()->GetHash() != (*headers.begin())->GetPreviousBlockHash())
	{
		if (!pSyncChain->Rewind(firstHeaderHeight - 1))
		{
			return EBlockChainStatus::UNKNOWN_ERROR;
		}
	}

	for (const BlockHeader* pHeader : headers)
	{
		// Add to chain
		BlockIndex* pBlockIndex = pLockedState->GetChainStore()->GetOrCreateIndex(pHeader->GetHash(), pHeader->GetHeight());
		pSyncChain->AddBlock(pBlockIndex);
	}

	return EBlockChainStatus::SUCCESS;
}

bool BlockHeaderProcessor::CheckAndAcceptSyncChain(Writer<ChainState> pLockedState) const
{
	BlockIndex* pSyncTip = pLockedState->GetChainStore()->GetSyncChain()->GetTip();
	std::unique_ptr<BlockHeader> pSyncHead = pLockedState->GetBlockDB()->GetBlockHeader(pSyncTip->GetHash());

	BlockIndex* pCandidateTip = pLockedState->GetChainStore()->GetCandidateChain()->GetTip();
	std::unique_ptr<BlockHeader> pCandidateHead = pLockedState->GetBlockDB()->GetBlockHeader(pCandidateTip->GetHash());

	if (pSyncHead == nullptr || pCandidateHead == nullptr)
	{
		return false;
	}

	if (pSyncHead->GetTotalDifficulty() > pCandidateHead->GetTotalDifficulty())
	{
		return pLockedState->GetChainStore()->ReorgChain(EChainType::SYNC, EChainType::CANDIDATE, pSyncTip->GetHeight());
	}

	return false;
}