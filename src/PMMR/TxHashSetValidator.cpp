#include "TxHashSetValidator.h"
#include "Common/MMR.h"
#include "Common/MMRHashUtil.h"
#include "Common/MMRUtil.h"
#include "KernelMMR.h"
#include "TxHashSetImpl.h"

#include <BlockChain/BlockChainServer.h>
#include <Common/Util/HexUtil.h>
#include <Consensus/Common.h>
#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Infrastructure/Logger.h>
#include <thread>

TxHashSetValidator::TxHashSetValidator(const IBlockChainServer &blockChainServer) : m_blockChainServer(blockChainServer)
{
}

std::unique_ptr<BlockSums> TxHashSetValidator::Validate(TxHashSet &txHashSet, const BlockHeader &blockHeader,
                                                        SyncStatus &syncStatus) const
{
    const KernelMMR &kernelMMR = *txHashSet.GetKernelMMR();
    const OutputPMMR &outputPMMR = *txHashSet.GetOutputPMMR();
    const RangeProofPMMR &rangeProofPMMR = *txHashSet.GetRangeProofPMMR();

    // Validate size of each MMR matches blockHeader
    if (!ValidateSizes(txHashSet, blockHeader))
    {
        LOG_ERROR("Invalid MMR size");
        return std::unique_ptr<BlockSums>(nullptr);
    }

    syncStatus.UpdateProcessingStatus(5);

    // Validate MMR hashes in parallel
    std::vector<std::thread> threads;
    std::atomic_bool mmrHashesValidated = true;
    threads.emplace_back(std::thread([this, &kernelMMR, &mmrHashesValidated] {
        if (!this->ValidateMMRHashes(kernelMMR))
        {
            mmrHashesValidated = false;
        }
    }));
    threads.emplace_back(std::thread([this, &outputPMMR, &mmrHashesValidated] {
        if (!this->ValidateMMRHashes(outputPMMR))
        {
            mmrHashesValidated = false;
        }
    }));
    threads.emplace_back(std::thread([this, &rangeProofPMMR, &mmrHashesValidated] {
        if (!this->ValidateMMRHashes(rangeProofPMMR))
        {
            mmrHashesValidated = false;
        }
    }));

    for (auto &thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    if (!mmrHashesValidated)
    {
        LOG_ERROR("Invalid MMR hashes");
        return std::unique_ptr<BlockSums>(nullptr);
    }

    syncStatus.UpdateProcessingStatus(10);

    // Validate root for each MMR matches blockHeader
    if (!txHashSet.ValidateRoots(blockHeader))
    {
        LOG_ERROR("Invalid MMR roots");
        return std::unique_ptr<BlockSums>(nullptr);
    }

    syncStatus.UpdateProcessingStatus(15);

    // Validate the full kernel history (kernel MMR root for every block header).
    LOG_DEBUG("Validating kernel history");
    if (!ValidateKernelHistory(*txHashSet.GetKernelMMR(), blockHeader, syncStatus))
    {
        LOG_ERROR("Invalid kernel history");
        return std::unique_ptr<BlockSums>(nullptr);
    }

    syncStatus.UpdateProcessingStatus(25);

    // Validate kernel sums
    LOG_DEBUG("Validating kernel sums");
    LoggerAPI::Flush();
    std::unique_ptr<BlockSums> pBlockSums = ValidateKernelSums(txHashSet, blockHeader);
    if (pBlockSums == nullptr)
    {
        LOG_ERROR("Invalid kernel sums");
        return std::unique_ptr<BlockSums>(nullptr);
    }

    syncStatus.UpdateProcessingStatus(40);

    // Validate the rangeproof associated with each unspent output.
    LOG_DEBUG("Validating range proofs");
    LoggerAPI::Flush();
    if (!ValidateRangeProofs(txHashSet, blockHeader, syncStatus))
    {
        LOG_ERROR("Failed to verify rangeproofs");
        return std::unique_ptr<BlockSums>(nullptr);
    }

    syncStatus.UpdateProcessingStatus(70);

    // Validate kernel signatures
    LOG_DEBUG("Validating kernel signatures");
    LoggerAPI::Flush();
    if (!ValidateKernelSignatures(*txHashSet.GetKernelMMR(), syncStatus))
    {
        LOG_ERROR("Failed to verify kernel signatures");
        return std::unique_ptr<BlockSums>(nullptr);
    }

    LOG_DEBUG("Success");
    LoggerAPI::Flush();

    syncStatus.UpdateProcessingStatus(100);

    return pBlockSums;
}

bool TxHashSetValidator::ValidateSizes(TxHashSet &txHashSet, const BlockHeader &blockHeader) const
{
    if (txHashSet.GetKernelMMR()->GetSize() != blockHeader.GetKernelMMRSize())
    {
        LOG_ERROR_F("Kernel size not matching for header (%s)", blockHeader.GetHash().ToHex().c_str());
        return false;
    }

    if (txHashSet.GetOutputPMMR()->GetSize() != blockHeader.GetOutputMMRSize())
    {
        LOG_ERROR_F("Output size not matching for header (%s)", blockHeader.GetHash().ToHex().c_str());
        return false;
    }

    if (txHashSet.GetRangeProofPMMR()->GetSize() != blockHeader.GetOutputMMRSize())
    {
        LOG_ERROR_F("RangeProof size not matching for header (%s)", blockHeader.GetHash().ToHex().c_str());
        return false;
    }

    return true;
}

// TODO: This probably belongs in MMRHashUtil.
bool TxHashSetValidator::ValidateMMRHashes(const MMR &mmr) const
{
    const uint64_t size = mmr.GetSize();
    for (uint64_t i = 0; i < size; i++)
    {
        const uint64_t height = MMRUtil::GetHeight(i);
        if (height > 0)
        {
            const std::unique_ptr<Hash> pParentHash = mmr.GetHashAt(i);
            if (pParentHash != nullptr)
            {
                const uint64_t leftIndex = MMRUtil::GetLeftChildIndex(i, height);
                const std::unique_ptr<Hash> pLeftHash = mmr.GetHashAt(leftIndex);

                const uint64_t rightIndex = MMRUtil::GetRightChildIndex(i);
                const std::unique_ptr<Hash> pRightHash = mmr.GetHashAt(rightIndex);

                if (pLeftHash != nullptr && pRightHash != nullptr)
                {
                    const Hash expectedHash = MMRHashUtil::HashParentWithIndex(*pLeftHash, *pRightHash, i);
                    if (*pParentHash != expectedHash)
                    {
                        LOG_ERROR_F("Invalid parent hash at index (%llu)", i);
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

bool TxHashSetValidator::ValidateKernelHistory(const KernelMMR &kernelMMR, const BlockHeader &blockHeader,
                                               SyncStatus &syncStatus) const
{
    const uint64_t totalHeight = blockHeader.GetHeight();
    for (uint64_t height = 0; height <= totalHeight; height++)
    {
        std::unique_ptr<BlockHeader> pHeader = m_blockChainServer.GetBlockHeaderByHeight(height, EChainType::CANDIDATE);
        if (pHeader == nullptr)
        {
            LOG_ERROR_F("No header found at height (%llu)", height);
            return false;
        }

        if (kernelMMR.Root(pHeader->GetKernelMMRSize()) != pHeader->GetKernelRoot())
        {
            LOG_ERROR_F("Kernel root not matching for header at height (%llu)", height);
            return false;
        }

        if (height % 1000 == 0)
        {
            syncStatus.UpdateProcessingStatus((uint8_t)(15 + ((10.0 * height) / totalHeight)));
        }
    }

    return true;
}

std::unique_ptr<BlockSums> TxHashSetValidator::ValidateKernelSums(TxHashSet &txHashSet,
                                                                  const BlockHeader &blockHeader) const
{
    // Calculate overage
    const int64_t overage = 0 - (Consensus::REWARD * (1 + blockHeader.GetHeight()));

    // Determine output commitments
    OutputPMMR *pOutputPMMR = txHashSet.GetOutputPMMR();
    std::vector<Commitment> outputCommitments;
    for (uint64_t i = 0; i < blockHeader.GetOutputMMRSize(); i++)
    {
        std::unique_ptr<OutputIdentifier> pOutput = pOutputPMMR->GetAt(i);
        if (pOutput != nullptr)
        {
            outputCommitments.push_back(pOutput->GetCommitment());
        }
    }

    // Determine kernel excess commitments
    KernelMMR *pKernelMMR = txHashSet.GetKernelMMR();
    std::vector<Commitment> excessCommitments;
    for (uint64_t i = 0; i < blockHeader.GetKernelMMRSize(); i++)
    {
        std::unique_ptr<TransactionKernel> pKernel = pKernelMMR->GetKernelAt(i);
        if (pKernel != nullptr)
        {
            excessCommitments.push_back(pKernel->GetExcessCommitment());
        }
    }

    return KernelSumValidator::ValidateKernelSums(std::vector<Commitment>(), outputCommitments, excessCommitments,
                                                  overage, blockHeader.GetTotalKernelOffset(), std::nullopt);
}

bool TxHashSetValidator::ValidateRangeProofs(TxHashSet &txHashSet, const BlockHeader &blockHeader,
                                             SyncStatus &syncStatus) const
{
    std::vector<std::pair<Commitment, RangeProof>> rangeProofs;

    size_t i = 0;
    LOG_INFO("BEGIN");
    const uint64_t outputMMRSize = txHashSet.GetOutputPMMR()->GetSize();
    for (uint64_t mmrIndex = 0; mmrIndex < outputMMRSize; mmrIndex++)
    {
        std::unique_ptr<OutputIdentifier> pOutput = txHashSet.GetOutputPMMR()->GetAt(mmrIndex);
        if (pOutput != nullptr)
        {
            std::unique_ptr<RangeProof> pRangeProof = txHashSet.GetRangeProofPMMR()->GetAt(mmrIndex);
            if (pRangeProof == nullptr)
            {
                LOG_ERROR_F("No rangeproof found at mmr index (%llu)", mmrIndex);
                return false;
            }

            rangeProofs.emplace_back(
                std::make_pair<Commitment, RangeProof>(Commitment(pOutput->GetCommitment()), RangeProof(*pRangeProof)));
            ++i;

            if (rangeProofs.size() >= 1000)
            {
                if (!Crypto::VerifyRangeProofs(rangeProofs))
                {
                    return false;
                }

                rangeProofs.clear();

                syncStatus.UpdateProcessingStatus((uint8_t)(40 + ((30.0 * mmrIndex) / outputMMRSize)));
            }
        }
    }

    if (!rangeProofs.empty())
    {
        if (!Crypto::VerifyRangeProofs(rangeProofs))
        {
            return false;
        }
    }

    LOG_INFO_F("SUCCESS (%llu)", i);
    return true;
}

bool TxHashSetValidator::ValidateKernelSignatures(const KernelMMR &kernelMMR, SyncStatus &syncStatus) const
{
    std::vector<TransactionKernel> kernels;

    const uint64_t mmrSize = kernelMMR.GetSize();
    for (uint64_t i = 0; i < mmrSize; i++)
    {
        std::unique_ptr<TransactionKernel> pKernel = kernelMMR.GetKernelAt(i);
        if (pKernel != nullptr)
        {
            kernels.push_back(*pKernel);

            if (kernels.size() >= 2000)
            {
                if (!KernelSignatureValidator::VerifyKernelSignatures(kernels))
                {
                    return false;
                }

                kernels.clear();

                syncStatus.UpdateProcessingStatus((uint8_t)(70 + ((30.0 * i) / mmrSize)));
            }
        }
    }

    if (!kernels.empty())
    {
        if (!KernelSignatureValidator::VerifyKernelSignatures(kernels))
        {
            return false;
        }
    }

    return true;
}