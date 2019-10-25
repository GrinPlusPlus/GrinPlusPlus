#include "TxHashSetImpl.h"
#include "Common/MMRHashUtil.h"
#include "Common/MMRUtil.h"
#include "TxHashSetValidator.h"

#include <BlockChain/BlockChainServer.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Database/BlockDb.h>
#include <Infrastructure/Logger.h>
#include <P2P/SyncStatus.h>
#include <thread>

TxHashSet::TxHashSet(IBlockDB &blockDB, KernelMMR *pKernelMMR, OutputPMMR *pOutputPMMR, RangeProofPMMR *pRangeProofPMMR,
                     const BlockHeader &blockHeader)
    : m_blockDB(blockDB), m_pKernelMMR(pKernelMMR), m_pOutputPMMR(pOutputPMMR), m_pRangeProofPMMR(pRangeProofPMMR),
      m_blockHeader(blockHeader), m_blockHeaderBackup(blockHeader)
{
}

TxHashSet::~TxHashSet()
{
    std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

    delete m_pKernelMMR;
    delete m_pOutputPMMR;
    delete m_pRangeProofPMMR;
}

bool TxHashSet::IsUnspent(const OutputLocation &location) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    return m_pOutputPMMR->IsUnpruned(location.GetMMRIndex());
}

bool TxHashSet::IsValid(const Transaction &transaction) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    // Validate inputs
    const uint64_t maximumBlockHeight =
        (std::max)(m_blockHeader.GetHeight() + 1, Consensus::COINBASE_MATURITY) - Consensus::COINBASE_MATURITY;
    for (const TransactionInput &input : transaction.GetBody().GetInputs())
    {
        const Commitment &commitment = input.GetCommitment();
        std::unique_ptr<OutputLocation> pOutputPosition = m_blockDB.GetOutputPosition(commitment);
        if (pOutputPosition == nullptr)
        {
            return false;
        }

        std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetMMRIndex());
        if (pOutput == nullptr || pOutput->GetCommitment() != commitment ||
            pOutput->GetFeatures() != input.GetFeatures())
        {
            LOG_DEBUG_F("Output (%s) not found at mmrIndex (%llu)", commitment.ToHex().c_str(),
                        pOutputPosition->GetMMRIndex());
            return false;
        }

        if (input.GetFeatures() == EOutputFeatures::COINBASE_OUTPUT)
        {
            if (pOutputPosition->GetBlockHeight() > maximumBlockHeight)
            {
                LOG_INFO_F("Coinbase (%s) not mature", transaction.GetHash().ToHex().c_str());
                return false;
            }
        }
    }

    // Validate outputs
    for (const TransactionOutput &output : transaction.GetBody().GetOutputs())
    {
        std::unique_ptr<OutputLocation> pOutputPosition = m_blockDB.GetOutputPosition(output.GetCommitment());
        if (pOutputPosition != nullptr)
        {
            std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetMMRIndex());
            if (pOutput != nullptr && pOutput->GetCommitment() == output.GetCommitment())
            {
                return false;
            }
        }
    }

    return true;
}

std::unique_ptr<BlockSums> TxHashSet::ValidateTxHashSet(const BlockHeader &header,
                                                        const IBlockChainServer &blockChainServer,
                                                        SyncStatus &syncStatus)
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    std::unique_ptr<BlockSums> pBlockSums = nullptr;

    try
    {
        LOG_INFO("Validating TxHashSet for block " + header.GetHash().ToHex());
        pBlockSums = TxHashSetValidator(blockChainServer).Validate(*this, header, syncStatus);
        if (pBlockSums != nullptr)
        {
            LOG_INFO("Successfully validated TxHashSet");
        }
    }
    catch (...)
    {
        LOG_ERROR("Exception thrown while processing TxHashSet");
    }

    return pBlockSums;
}

bool TxHashSet::ApplyBlock(const FullBlock &block)
{
    std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

    Roaring blockInputBitmap;

    // Prune inputs
    for (const TransactionInput &input : block.GetTransactionBody().GetInputs())
    {
        const Commitment &commitment = input.GetCommitment();
        std::unique_ptr<OutputLocation> pOutputPosition = m_blockDB.GetOutputPosition(commitment);
        if (pOutputPosition == nullptr)
        {
            LOG_WARNING_F("Output position not found for commitment (%s) in block (%llu)", commitment.ToHex().c_str(),
                          block.GetBlockHeader().GetHeight());
            return false;
        }

        const uint64_t mmrIndex = pOutputPosition->GetMMRIndex();
        if (!m_pOutputPMMR->Remove(mmrIndex))
        {
            LOG_WARNING_F("Failed to remove output at position (%llu)", mmrIndex);
            return false;
        }

        if (!m_pRangeProofPMMR->Remove(mmrIndex))
        {
            LOG_WARNING_F("Failed to remove rangeproof at position (%llu)", mmrIndex);
            return false;
        }

        blockInputBitmap.add(mmrIndex + 1);
    }

    m_blockDB.AddBlockInputBitmap(block.GetBlockHeader().GetHash(), blockInputBitmap);

    // Append new outputs
    for (const TransactionOutput &output : block.GetTransactionBody().GetOutputs())
    {
        std::unique_ptr<OutputLocation> pOutputPosition = m_blockDB.GetOutputPosition(output.GetCommitment());
        if (pOutputPosition != nullptr)
        {
            if (pOutputPosition->GetMMRIndex() < m_blockHeader.GetOutputMMRSize())
            {
                std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetMMRIndex());
                if (pOutput != nullptr && pOutput->GetCommitment() == output.GetCommitment())
                {
                    return false;
                }
            }
        }

        const uint64_t mmrIndex = m_pOutputPMMR->GetSize();
        const uint64_t blockHeight = block.GetBlockHeader().GetHeight();
        if (!m_pOutputPMMR->Append(OutputIdentifier::FromOutput(output)))
        {
            return false;
        }

        if (!m_pRangeProofPMMR->Append(output.GetRangeProof()))
        {
            return false;
        }

        m_blockDB.AddOutputPosition(output.GetCommitment(), OutputLocation(mmrIndex, blockHeight));
    }

    // Append new kernels
    for (const TransactionKernel &kernel : block.GetTransactionBody().GetKernels())
    {
        m_pKernelMMR->ApplyKernel(kernel);
    }

    m_blockHeader = block.GetBlockHeader();

    return true;
}

bool TxHashSet::ValidateRoots(const BlockHeader &blockHeader) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    if (m_pKernelMMR->Root(blockHeader.GetKernelMMRSize()) != blockHeader.GetKernelRoot())
    {
        LOG_ERROR_F("Kernel root not matching for header (%s)", blockHeader.GetHash().ToHex().c_str());
        return false;
    }

    if (m_pOutputPMMR->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetOutputRoot())
    {
        LOG_ERROR_F("Output root not matching for header (%s)", blockHeader.GetHash().ToHex().c_str());
        return false;
    }

    if (m_pRangeProofPMMR->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetRangeProofRoot())
    {
        LOG_ERROR_F("RangeProof root not matching for header (%s)", blockHeader.GetHash().ToHex().c_str());
        return false;
    }

    return true;
}

bool TxHashSet::SaveOutputPositions(const BlockHeader &blockHeader, const uint64_t firstOutputIndex)
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    const uint64_t size = blockHeader.GetOutputMMRSize();
    for (uint64_t mmrIndex = firstOutputIndex; mmrIndex < size; mmrIndex++)
    {
        std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(mmrIndex);
        if (pOutput != nullptr)
        {
            m_blockDB.AddOutputPosition(pOutput->GetCommitment(), OutputLocation(mmrIndex, blockHeader.GetHeight()));
        }
    }

    return true;
}

std::vector<Hash> TxHashSet::GetLastKernelHashes(const uint64_t numberOfKernels) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    return m_pKernelMMR->GetLastLeafHashes(numberOfKernels);
}

std::vector<Hash> TxHashSet::GetLastOutputHashes(const uint64_t numberOfOutputs) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    return m_pOutputPMMR->GetLastLeafHashes(numberOfOutputs);
}

std::vector<Hash> TxHashSet::GetLastRangeProofHashes(const uint64_t numberOfRangeProofs) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    return m_pRangeProofPMMR->GetLastLeafHashes(numberOfRangeProofs);
}

OutputRange TxHashSet::GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    const uint64_t outputSize = m_pOutputPMMR->GetSize();

    uint64_t leafIndex = startIndex;
    std::vector<OutputDisplayInfo> outputs;
    outputs.reserve(maxNumOutputs);
    while (outputs.size() < maxNumOutputs)
    {
        const uint64_t mmrIndex = MMRUtil::GetPMMRIndex(leafIndex++);
        if (mmrIndex >= outputSize)
        {
            break;
        }

        std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(mmrIndex);
        if (pOutput != nullptr)
        {
            std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(mmrIndex);
            std::unique_ptr<OutputLocation> pOutputPosition = m_blockDB.GetOutputPosition(pOutput->GetCommitment());
            if (pRangeProof == nullptr || pOutputPosition == nullptr || pOutputPosition->GetMMRIndex() != mmrIndex)
            {
                return OutputRange(0, 0, std::vector<OutputDisplayInfo>()); // TODO: Throw exception
            }

            outputs.emplace_back(OutputDisplayInfo(false, *pOutput, *pOutputPosition, *pRangeProof));
        }
    }

    const uint64_t maxLeafIndex = MMRUtil::GetNumLeaves(outputSize - 1);
    const uint64_t lastRetrievedIndex =
        outputs.empty() ? 0 : MMRUtil::GetNumLeaves(outputs.back().GetLocation().GetMMRIndex());

    return OutputRange(maxLeafIndex, lastRetrievedIndex, std::move(outputs));
}

std::vector<OutputDisplayInfo> TxHashSet::GetOutputsByMMRIndex(const uint64_t startIndex,
                                                               const uint64_t lastIndex) const
{
    std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

    std::vector<OutputDisplayInfo> outputs;
    uint64_t mmrIndex = startIndex;
    while (mmrIndex <= lastIndex)
    {
        std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(mmrIndex);
        if (pOutput != nullptr)
        {
            std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(mmrIndex);
            std::unique_ptr<OutputLocation> pOutputPosition = m_blockDB.GetOutputPosition(pOutput->GetCommitment());

            outputs.emplace_back(OutputDisplayInfo(false, *pOutput, *pOutputPosition, *pRangeProof));
        }

        ++mmrIndex;
    }

    return outputs;
}

bool TxHashSet::Rewind(const BlockHeader &header)
{
    std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

    Roaring leavesToAdd;
    while (m_blockHeader != header)
    {
        std::unique_ptr<Roaring> pBlockInputBitmap = m_blockDB.GetBlockInputBitmap(m_blockHeader.GetHash());
        if (pBlockInputBitmap != nullptr)
        {
            leavesToAdd |= *pBlockInputBitmap;
        }
        else
        {
            return false;
        }

        m_blockHeader = *m_blockDB.GetBlockHeader(m_blockHeader.GetPreviousBlockHash());
    }

    m_pKernelMMR->Rewind(header.GetKernelMMRSize());
    m_pOutputPMMR->Rewind(header.GetOutputMMRSize(), leavesToAdd);
    m_pRangeProofPMMR->Rewind(header.GetOutputMMRSize(), leavesToAdd);

    return true;
}

bool TxHashSet::Commit()
{
    std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

    std::vector<std::thread> threads;
    threads.emplace_back(std::thread([this] { this->m_pKernelMMR->Flush(); }));
    threads.emplace_back(std::thread([this] { this->m_pOutputPMMR->Flush(); }));
    threads.emplace_back(std::thread([this] { this->m_pRangeProofPMMR->Flush(); }));
    for (auto &thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    m_blockHeaderBackup = m_blockHeader;
    return true;
}

bool TxHashSet::Discard()
{
    std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

    m_pKernelMMR->Discard();
    m_pOutputPMMR->Discard();
    m_pRangeProofPMMR->Discard();
    m_blockHeader = m_blockHeaderBackup;
    return true;
}

bool TxHashSet::Compact()
{
    std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

    // TODO: Implement

    return true;
}