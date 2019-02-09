#include "BlockHeaderProcessor.h"
#include "../Validators/BlockHeaderValidator.h"

#include <Infrastructure/Logger.h>
#include <HeaderMMR.h>
#include <HexUtil.h>
#include <StringUtil.h>

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
		LoggerAPI::LogDebug(StringUtil::Format("BlockHeaderProcessor::ProcessSingleHeader - Processing header %s as an orphan.", header.FormatHash().c_str()));
		return EBlockChainStatus::ORPHANED;
	}

	LoggerAPI::LogTrace("BlockHeaderProcessor::ProcessSingleHeader - Processing next candidate header " + header.FormatHash());

	// Validate the header.
	std::unique_ptr<BlockHeader> pPreviousHeaderPtr = lockedState.m_blockStore.GetBlockHeaderByHash(pLastIndex->GetHash());
	if (!BlockHeaderValidator(m_config, lockedState.m_headerMMR).IsValidHeader(header, *pPreviousHeaderPtr))
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

	std::vector<BlockHeader> chunkedHeaders;
	chunkedHeaders.reserve(32);
	while (index < size)
	{
		chunkedHeaders.push_back(headers[index++]);
		if (index % 32 == 0)
		{
			const EBlockChainStatus processChunkStatus = ProcessChunkedSyncHeaders(chunkedHeaders);
			if (processChunkStatus != EBlockChainStatus::SUCCESS && processChunkStatus != EBlockChainStatus::ALREADY_EXISTS)
			{
				return processChunkStatus;
			}

			chunkedHeaders.clear();
		}
	}

	if (!chunkedHeaders.empty())
	{
		return ProcessChunkedSyncHeaders(chunkedHeaders);
	}

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessChunkedSyncHeaders(const std::vector<BlockHeader>& headers)
{
	LockedChainState lockedState = m_chainState.GetLocked();
	Chain& syncChain = lockedState.m_chainStore.GetSyncChain();

	// Filter out headers that are already part of sync chain.
	std::vector<BlockHeader> newHeaders;
	for (size_t i = 0; i < headers.size(); i++)
	{
		const BlockHeader& header = headers[i];
		BlockIndex* pSyncHeader = syncChain.GetByHeight(header.GetHeight());
		if (pSyncHeader == nullptr || header.GetHash() != pSyncHeader->GetHash())
		{
			newHeaders.push_back(header);
		}
	}

	if (newHeaders.empty())
	{
		LoggerAPI::LogDebug("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Headers already processed.");
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	LoggerAPI::LogInfo("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Processing " + std::to_string(newHeaders.size()) + " headers.");

	// Check if previous header exists and matches previous.
	BlockIndex* pPrevIndex = syncChain.GetByHeight(newHeaders.front().GetHeight() - 1);
	if (pPrevIndex == nullptr || pPrevIndex->GetHash() != newHeaders.front().GetPreviousBlockHash())
	{
		LoggerAPI::LogInfo("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Previous header doesn't match. Still syncing?");
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	// Rewind MMR
	IHeaderMMR& headerMMR = lockedState.m_headerMMR;

	headerMMR.Rewind(newHeaders.front().GetHeight());

	// Validate the headers.
	std::unique_ptr<BlockHeader> pPreviousHeaderPtr = lockedState.m_blockStore.GetBlockHeaderByHash(pPrevIndex->GetHash());
	const BlockHeader* pPreviousHeader = pPreviousHeaderPtr.get();
	for (auto& header : newHeaders)
	{
		if (!BlockHeaderValidator(m_config, headerMMR).IsValidHeader(header, *pPreviousHeader))
		{
			headerMMR.Rollback();
			return EBlockChainStatus::INVALID;
		}

		headerMMR.AddHeader(header);
		pPreviousHeader = &header;
	}

	// Add the headers to the chain state.
	const EBlockChainStatus addSyncHeadersStatus = AddSyncHeaders(lockedState, newHeaders);
	if (addSyncHeadersStatus != EBlockChainStatus::SUCCESS)
	{
		LoggerAPI::LogError("BlockHeaderProcessor::ProcessChunkedSyncHeaders - Failed to add sync headers.");
		headerMMR.Rollback();
		// TODO: Copy header chain into sync chain
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

EBlockChainStatus BlockHeaderProcessor::AddSyncHeaders(LockedChainState& lockedState, const std::vector<BlockHeader>& headers) const
{
	Chain& syncChain = lockedState.m_chainStore.GetSyncChain();
	const uint64_t firstHeaderHeight = headers.begin()->GetHeight();

	// Ensure chain is on correct fork.
	BlockIndex* pPrevious = syncChain.GetByHeight(firstHeaderHeight - 1);
	if (pPrevious == nullptr || pPrevious->GetHash() != headers.begin()->GetPreviousBlockHash())
	{
		return EBlockChainStatus::UNKNOWN_ERROR;
	}

	// Rewind chain if necessary.
	if (syncChain.GetTip()->GetHash() != headers.begin()->GetPreviousBlockHash())
	{
		if (!syncChain.Rewind(firstHeaderHeight - 1))
		{
			return EBlockChainStatus::UNKNOWN_ERROR;
		}
	}

	std::vector<BlockHeader*> blockHeadersToAdd;
	for (auto& header : headers)
	{
		// Add to chain
		BlockIndex* pBlockIndex = lockedState.m_chainStore.GetOrCreateIndex(header.GetHash(), header.GetHeight(), pPrevious);
		syncChain.AddBlock(pBlockIndex);
		pPrevious = pBlockIndex;
	}

	lockedState.m_blockStore.AddHeaders(headers);

	return EBlockChainStatus::SUCCESS;
}

bool BlockHeaderProcessor::CheckAndAcceptSyncChain(LockedChainState& lockedState) const
{
	Chain& syncChain = lockedState.m_chainStore.GetSyncChain();
	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();

	const Hash syncHeadHash = syncChain.GetTip()->GetHash();
	std::unique_ptr<BlockHeader> pSyncHead = lockedState.m_blockStore.GetBlockHeaderByHash(syncHeadHash);

	const Hash candidateHeadHash = candidateChain.GetTip()->GetHash();
	std::unique_ptr<BlockHeader> pCandidateHead = lockedState.m_blockStore.GetBlockHeaderByHash(candidateHeadHash);

	if (pSyncHead == nullptr || pCandidateHead == nullptr)
	{
		return false;
	}

	if (pSyncHead->GetTotalDifficulty() > pCandidateHead->GetTotalDifficulty())
	{
		BlockIndex* pCommonIndex = lockedState.m_chainStore.FindCommonIndex(EChainType::SYNC, EChainType::CANDIDATE);
		if (candidateChain.Rewind(pCommonIndex->GetHeight()))
		{
			uint64_t height = pCommonIndex->GetHeight() + 1;
			while (height <= pSyncHead->GetHeight())
			{
				candidateChain.AddBlock(syncChain.GetByHeight(height));
				height++;
			}

			return true;
		}
	}

	return false;
}