#include "BlockHeaderProcessor.h"
#include "../Validators/BlockHeaderValidator.h"

#include <Infrastructure/Logger.h>
#include <PMMR/HeaderMMR.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>

static const size_t SYNC_BATCH_SIZE = 32;

BlockHeaderProcessor::BlockHeaderProcessor(const Config& config, ChainState& chainState)
	: m_config(config), m_chainState(chainState)
{

}

EBlockChainStatus BlockHeaderProcessor::ProcessSingleHeader(const BlockHeader& header)
{
	LoggerAPI::LogTrace("BlockHeaderProcessor::ProcessSingleHeader - Validating " + header.FormatHash());

	LockedChainState lockedState = m_chainState.GetLocked();
	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();

	// Check if header already processed
	BlockIndex* pCandidateIndex = candidateChain.GetByHeight(header.GetHeight());
	if (pCandidateIndex != nullptr && pCandidateIndex->GetHash() == header.GetHash())
	{
		LoggerAPI::LogTrace(StringUtil::Format("BlockHeaderProcessor::ProcessSingleHeader - Header %s already processed.", header.FormatHash().c_str()));
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// If this is not the next header needed, process as an orphan.
	BlockIndex* pLastIndex = candidateChain.GetTip();
	if (pLastIndex->GetHash() != header.GetPreviousBlockHash())
	{
		std::unique_ptr<BlockHeader> pCandidateHeader = lockedState.m_blockStore.GetBlockHeaderByHash(pLastIndex->GetHash());
		if (header.GetTotalDifficulty() > pCandidateHeader->GetTotalDifficulty())
		{
			std::vector<BlockHeader> headers;
			std::unique_ptr<BlockHeader> pHeader = std::make_unique<BlockHeader>(BlockHeader(header));
			while (pHeader != nullptr)
			{
				headers.push_back(*pHeader);
				BlockIndex* pIndex = candidateChain.GetByHeight(pHeader->GetHeight());
				if (pIndex != nullptr && pIndex->GetHash() == pHeader->GetHash())
				{
					// All headers exist. Reorg.
					std::vector<const BlockHeader*> reorgHeaders;
					for (auto iter = headers.rbegin(); iter != headers.rend(); iter++)
					{
						const BlockHeader& reorgHeader = *iter;
						reorgHeaders.push_back(&reorgHeader);
					}

					return ProcessChunkedSyncHeaders(lockedState, reorgHeaders);
				}

				const Hash previousHash = pHeader->GetPreviousBlockHash();
				pHeader = lockedState.m_orphanPool.GetOrphanHeader(previousHash);
				if (pHeader == nullptr)
				{
					pHeader = lockedState.m_blockStore.GetBlockHeaderByHash(previousHash);
				}
			}
		}

		LoggerAPI::LogDebug(StringUtil::Format("BlockHeaderProcessor::ProcessSingleHeader - Processing header %s as an orphan.", header.FormatHash().c_str()));
		lockedState.m_orphanPool.AddOrphanHeader(header);
		return EBlockChainStatus::ORPHANED;
	}

	LoggerAPI::LogTrace("BlockHeaderProcessor::ProcessSingleHeader - Processing next candidate header " + header.FormatHash());

	// Validate the header.
	std::unique_ptr<BlockHeader> pPreviousHeaderPtr = lockedState.m_blockStore.GetBlockHeaderByHash(pLastIndex->GetHash());
	if (!BlockHeaderValidator(m_config, lockedState.m_blockStore.GetBlockDB(), lockedState.m_headerMMR).IsValidHeader(header, *pPreviousHeaderPtr))
	{
		LoggerAPI::LogError("BlockHeaderProcessor::ProcessSingleHeader - Header failed to validate.");
		return EBlockChainStatus::INVALID;
	}

	lockedState.m_blockStore.AddHeader(header);
	lockedState.m_headerMMR.AddHeader(header);
	lockedState.m_headerMMR.Commit();

	BlockIndex* pBlockIndex = lockedState.m_chainStore.GetOrCreateIndex(header.GetHash(), header.GetHeight(), pLastIndex);
	lockedState.m_chainStore.GetSyncChain().AddBlock(pBlockIndex);
	candidateChain.AddBlock(pBlockIndex);
	lockedState.m_chainStore.Flush();

	LoggerAPI::LogDebug("BlockHeaderProcessor::ProcessSingleHeader - Successfully validated " + header.FormatHash());

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
		LoggerAPI::LogError("BlockHeaderProcessor::ProcessSyncHeaders - Header with height 0 received.");
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
			LockedChainState lockedState = m_chainState.GetLocked();
			const EBlockChainStatus processChunkStatus = ProcessChunkedSyncHeaders(lockedState, chunkedHeaders);
			if (processChunkStatus != EBlockChainStatus::SUCCESS && processChunkStatus != EBlockChainStatus::ALREADY_EXISTS)
			{
				return processChunkStatus;
			}

			chunkedHeaders.clear();
		}
	}

	if (!chunkedHeaders.empty())
	{
		LockedChainState lockedState = m_chainState.GetLocked();
		return ProcessChunkedSyncHeaders(lockedState, chunkedHeaders);
	}

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessChunkedSyncHeaders(LockedChainState& lockedState, const std::vector<const BlockHeader*>& headers)
{
	Chain& syncChain = lockedState.m_chainStore.GetSyncChain();
	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();

	// Filter out headers that are already part of sync chain.
	std::vector<const BlockHeader*> newHeaders;
	for (size_t i = 0; i < headers.size(); i++)
	{
		const BlockHeader* pHeader = headers[i];
		BlockIndex* pSyncHeader = syncChain.GetByHeight(pHeader->GetHeight());
		if (pSyncHeader == nullptr || pHeader->GetHash() != pSyncHeader->GetHash())
		{
			newHeaders.push_back(pHeader);
		}
	}

	if (newHeaders.empty())
	{
		LoggerAPI::LogTrace("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Headers already processed.");
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	LoggerAPI::LogTrace("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Processing " + std::to_string(newHeaders.size()) + " headers.");

	// Check if previous header exists and matches previous.
	BlockIndex* pPrevIndex = syncChain.GetByHeight(newHeaders.front()->GetHeight() - 1);
	if (pPrevIndex == nullptr || pPrevIndex->GetHash() != newHeaders.front()->GetPreviousBlockHash())
	{
		LoggerAPI::LogInfo("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Previous header doesn't match. Still syncing?");
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	// Rewind MMR
	IHeaderMMR& headerMMR = lockedState.m_headerMMR;
	headerMMR.Rewind(newHeaders.front()->GetHeight());

	// Validate the headers.
	std::unique_ptr<BlockHeader> pPreviousHeaderPtr = lockedState.m_blockStore.GetBlockHeaderByHash(pPrevIndex->GetHash());
	const BlockHeader* pPreviousHeader = pPreviousHeaderPtr.get();
	for (const BlockHeader* pHeader : newHeaders)
	{
		if (!BlockHeaderValidator(m_config, lockedState.m_blockStore.GetBlockDB(), headerMMR).IsValidHeader(*pHeader, *pPreviousHeader))
		{
			headerMMR.Rollback();
			return EBlockChainStatus::INVALID;
		}

		headerMMR.AddHeader(*pHeader);
		lockedState.m_blockStore.AddHeader(*pHeader);
		pPreviousHeader = pHeader;
	}

	// Add the headers to the chain state.
	const EBlockChainStatus addSyncHeadersStatus = AddSyncHeaders(lockedState, newHeaders);
	if (addSyncHeadersStatus != EBlockChainStatus::SUCCESS)
	{
		LoggerAPI::LogError("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Failed to add sync headers.");
		headerMMR.Rollback();
		lockedState.m_chainStore.ReorgChain(EChainType::CANDIDATE, EChainType::SYNC, candidateChain.GetTip()->GetHeight());

		return addSyncHeadersStatus;
	}

	// If total difficulty increases, accept sync chain as new candidate chain.
	if (CheckAndAcceptSyncChain(lockedState))
	{
		headerMMR.Commit();
	}
	else
	{
		headerMMR.Rollback();
	}

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::AddSyncHeaders(LockedChainState& lockedState, const std::vector<const BlockHeader*>& headers) const
{
	Chain& syncChain = lockedState.m_chainStore.GetSyncChain();
	const uint64_t firstHeaderHeight = (*headers.begin())->GetHeight();

	// Ensure chain is on correct fork.
	BlockIndex* pPrevious = syncChain.GetByHeight(firstHeaderHeight - 1);
	if (pPrevious == nullptr || pPrevious->GetHash() != (*headers.begin())->GetPreviousBlockHash())
	{
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	// Rewind chain if necessary.
	if (syncChain.GetTip()->GetHash() != (*headers.begin())->GetPreviousBlockHash())
	{
		if (!syncChain.Rewind(firstHeaderHeight - 1))
		{
			return EBlockChainStatus::UNKNOWN_ERROR;
		}
	}

	for (const BlockHeader* pHeader : headers)
	{
		// Add to chain
		BlockIndex* pBlockIndex = lockedState.m_chainStore.GetOrCreateIndex(pHeader->GetHash(), pHeader->GetHeight(), pPrevious);
		syncChain.AddBlock(pBlockIndex);
		pPrevious = pBlockIndex;
	}

	return EBlockChainStatus::SUCCESS;
}

bool BlockHeaderProcessor::CheckAndAcceptSyncChain(LockedChainState& lockedState) const
{
	BlockIndex* pSyncTip = lockedState.m_chainStore.GetSyncChain().GetTip();
	std::unique_ptr<BlockHeader> pSyncHead = lockedState.m_blockStore.GetBlockHeaderByHash(pSyncTip->GetHash());

	BlockIndex* pCandidateTip = lockedState.m_chainStore.GetCandidateChain().GetTip();
	std::unique_ptr<BlockHeader> pCandidateHead = lockedState.m_blockStore.GetBlockHeaderByHash(pCandidateTip->GetHash());

	if (pSyncHead == nullptr || pCandidateHead == nullptr)
	{
		return false;
	}

	if (pSyncHead->GetTotalDifficulty() > pCandidateHead->GetTotalDifficulty())
	{
		return lockedState.m_chainStore.ReorgChain(EChainType::SYNC, EChainType::CANDIDATE, pSyncTip->GetHeight());
	}

	return false;
}