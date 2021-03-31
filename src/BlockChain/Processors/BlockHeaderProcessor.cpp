#include "BlockHeaderProcessor.h"
#include "../Validators/BlockHeaderValidator.h"

#include <Database/BlockDb.h>
#include <Core/Exceptions/BadDataException.h>
#include <Core/Exceptions/BlockChainException.h>
#include <BlockChain/BadBlocks.h>
#include <Common/Logger.h>
#include <PMMR/HeaderMMR.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>

static const size_t SYNC_BATCH_SIZE = 128;

EBlockChainStatus BlockHeaderProcessor::ProcessSingleHeader(const BlockHeaderPtr& pHeader)
{
    LOG_TRACE_F("Validating {}", *pHeader);

    if (BAD_BLOCKS.find(pHeader->GetHash()) != BAD_BLOCKS.end()) {
        return EBlockChainStatus::INVALID;
    }

    auto pLockedState = m_pChainState->BatchWrite();
    auto pBlockDB = pLockedState->GetBlockDB();
    auto pHeaderMMR = pLockedState->GetHeaderMMR();
    auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

    // Check if header already processed
    if (pBlockDB->GetBlockHeader(pHeader->GetHash()) != nullptr) {
        LOG_TRACE_F("Header {} already processed.", *pHeader);
        return EBlockChainStatus::ALREADY_EXISTS;
    }

    // If this is not the next header needed, process as an orphan.
    if (pCandidateChain->GetTipHash() != pHeader->GetPreviousHash()) {
        LOG_INFO_F(
            "Processing orphan header {}. Candidate tip hash was {}",
            *pHeader,
            pCandidateChain->GetTipHash()
        );
        return ProcessOrphan(pLockedState, pHeader);
    }

    LOG_DEBUG_F("Processing next candidate header: {}", *pHeader);

    // Validate the header.
    auto pPreviousHeaderPtr = pBlockDB->GetBlockHeader(pCandidateChain->GetTipHash());
    BlockHeaderValidator(pBlockDB, pHeaderMMR).Validate(*pHeader, *pPreviousHeaderPtr);

    pBlockDB->AddBlockHeader(pHeader);
    pHeaderMMR->AddHeader(*pHeader);
    pCandidateChain->AddBlock(pHeader->GetHash(), pHeader->GetHeight());

    LOG_DEBUG_F("Successfully validated {}", *pHeader);

    pLockedState->Commit();
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
    while (pTempHeader != nullptr) {
        if (pCandidateChain->IsOnChain(pTempHeader)) {
            // All headers exist. Reorg.
            std::reverse(reorgHeaders.begin(), reorgHeaders.end());
            break;
        }

        reorgHeaders.push_back(pTempHeader);
        const Hash& previousHash = pTempHeader->GetPreviousHash();
        pTempHeader = pOrphanPool->GetOrphanHeader(previousHash);
        if (pTempHeader == nullptr) {
            pTempHeader = pBlockDB->GetBlockHeader(previousHash);
        }
    }

    if (pTempHeader == nullptr) {
        LOG_DEBUG_F("Processing header {} as an orphan.", *pHeader);
        pOrphanPool->AddOrphanHeader(pHeader);
        return EBlockChainStatus::ORPHANED;
    }

    // Rewind to fork point
    pCandidateChain->Rewind(reorgHeaders.front()->GetHeight() - 1);
    pHeaderMMR->Rewind(reorgHeaders.front()->GetHeight());

    // Validate each header and add it to the MMR & BlockDB
    ValidateHeaders(pLockedState, reorgHeaders);

    if (pHeader->GetTotalDifficulty() <= totalDifficulty) {
        // The header MMR should always match the candidate chain.
        // Since the total difficulty was not enough to reorg the candidate chain, we must rollback our MMR changes.
        LOG_INFO("Total difficulty did not increase. Rolling back header MMR changes.");
        pHeaderMMR->Rollback();
        pCandidateChain->Rollback();
    }

    pLockedState->Commit();
    return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessSyncHeaders(const std::vector<BlockHeaderPtr>& headers)
{
    if (headers.empty()) {
        return EBlockChainStatus::SUCCESS;
    }

    for (const auto& pHeader : headers) {
        if (BAD_BLOCKS.find(pHeader->GetHash()) != BAD_BLOCKS.end()) {
            throw BAD_DATA_EXCEPTION(EBanReason::BadBlockHeader, "Header included in BAD_BLOCKS");
        }
    }

    uint64_t height = headers.front()->GetHeight();
    if (height == 0) {
        throw BAD_DATA_EXCEPTION(EBanReason::BadBlockHeader, "Header with height 0 received.");
    }

    for (size_t i = 1; i < headers.size(); i++) {
        if (headers[i]->GetHeight() != (headers[i - 1]->GetHeight() + 1)) {
            throw BAD_DATA_EXCEPTION(EBanReason::BadBlockHeader, "Headers not sorted.");
        }
    }

    const size_t size = headers.size();
    size_t index = 0;

    std::vector<BlockHeaderPtr> chunkedHeaders;
    chunkedHeaders.reserve(SYNC_BATCH_SIZE);
    while (index < size) {
        chunkedHeaders.push_back(headers[index++]);
        if (index % SYNC_BATCH_SIZE == 0 || index == size) {
            const EBlockChainStatus processChunkStatus = ProcessChunkedSyncHeaders(chunkedHeaders);
            if (processChunkStatus != EBlockChainStatus::SUCCESS && processChunkStatus != EBlockChainStatus::ALREADY_EXISTS) {
                return processChunkStatus;
            }

            chunkedHeaders.clear();
        }
    }

    return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockHeaderProcessor::ProcessChunkedSyncHeaders(const std::vector<BlockHeaderPtr>& headers)
{
    auto pLockedState = m_pChainState->BatchWrite();
    auto pHeaderMMR = pLockedState->GetHeaderMMR();
    auto pChainStore = pLockedState->GetChainStore();
    auto pCandidateChain = pChainStore->GetCandidateChain();

    const uint64_t totalDifficulty = pLockedState->GetTotalDifficulty(EChainType::CANDIDATE);

    PrepareSyncChain(pLockedState, headers);

    // Filter out headers that are already part of sync chain.
    std::vector<BlockHeaderPtr> newHeaders;
    for (size_t i = 0; i < headers.size(); i++) {
        auto pHeader = headers[i];
        if (!newHeaders.empty() || !pCandidateChain->IsOnChain(pHeader)) {
            newHeaders.push_back(pHeader);
        }
    }

    if (newHeaders.empty()) {
        LOG_DEBUG("Headers already processed.");
        return EBlockChainStatus::ALREADY_EXISTS;
    }

    LOG_TRACE_F("Processing headers {} to {}.", *newHeaders.front(), *newHeaders.back());

    // Rewind MMR
    pCandidateChain->Rewind(newHeaders.front()->GetHeight() - 1);
    pHeaderMMR->Rewind(newHeaders.front()->GetHeight());

    // Validate the headers.
    ValidateHeaders(pLockedState, newHeaders);

    // If total difficulty increases, accept sync chain as new candidate chain.
    if (newHeaders.back()->GetTotalDifficulty() <= totalDifficulty) {
        // The header MMR should always match the candidate chain.
        // Since the total difficulty was not enough to reorg the candidate chain, we must rollback our MMR changes.
        LOG_INFO("Total difficulty did not increase. Rolling back header MMR changes.");
        pHeaderMMR->Rollback();
        pCandidateChain->Rollback();
    }

    pLockedState->Commit();
    return EBlockChainStatus::SUCCESS;
}

void BlockHeaderProcessor::PrepareSyncChain(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
    LOG_TRACE("Preparing candidate chain");

    auto pChainStore = pLockedState->GetChainStore();
    auto pCandidateChain = pChainStore->GetCandidateChain();

    // Check if previous header exists and matches previous hash.
    const uint64_t previousHeight = headers.front()->GetHeight() - 1;
    const Hash& previousHash = headers.front()->GetPreviousHash();
    if (!pCandidateChain->IsOnChain(previousHeight, previousHash)) {
        throw BLOCK_CHAIN_EXCEPTION("Previous header not found on chain.");
    }
}

void BlockHeaderProcessor::RewindMMR(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
    LOG_TRACE("Rewinding MMR");

    auto pHeaderMMR = pLockedState->GetHeaderMMR();
    pHeaderMMR->Rewind(headers.front()->GetHeight());
}

void BlockHeaderProcessor::ValidateHeaders(Writer<ChainState> pLockedState, const std::vector<BlockHeaderPtr>& headers)
{
    LOG_TRACE("Validating headers");

    auto pBlockDB = pLockedState->GetBlockDB();
    auto pHeaderMMR = pLockedState->GetHeaderMMR();
    auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();
    BlockHeaderValidator validator(pBlockDB, pHeaderMMR);

    const Hash& previousHash = headers.front()->GetPreviousHash();
    auto pPreviousHeader = pBlockDB->GetBlockHeader(previousHash);
    if (pPreviousHeader == nullptr) {
        throw BLOCK_CHAIN_EXCEPTION_F("Previous header ({}) not found.", previousHash);
    }

    for (const auto& pHeader : headers) {
        validator.Validate(*pHeader, *pPreviousHeader);

        pHeaderMMR->AddHeader(*pHeader);
        pBlockDB->AddBlockHeader(pHeader);
        pCandidateChain->AddBlock(pHeader->GetHash(), pHeader->GetHeight());
        pPreviousHeader = pHeader;
    }
}