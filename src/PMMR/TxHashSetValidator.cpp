#include "TxHashSetValidator.h"
#include "TxHashSetImpl.h"
#include "KernelMMR.h"
#include "Common/MMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Consensus.h>
#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Common/Util/HexUtil.h>
#include <Common/Logger.h>
#include <BlockChain/BlockChain.h>
#include <thread>

std::unique_ptr<BlockSums> TxHashSetValidator::Validate(TxHashSet& txHashSet, const BlockHeader& blockHeader, SyncStatus& syncStatus) const
{
	std::shared_ptr<const KernelMMR> pKernelMMR = txHashSet.GetKernelMMR();
	std::shared_ptr<const OutputPMMR> pOutputPMMR = txHashSet.GetOutputPMMR();
	std::shared_ptr<const RangeProofPMMR> pRangeProofPMMR = txHashSet.GetRangeProofPMMR();

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
	threads.emplace_back(std::thread([this, pKernelMMR, &mmrHashesValidated] { if (!this->ValidateMMRHashes(pKernelMMR)) { mmrHashesValidated = false; }}));
	threads.emplace_back(std::thread([this, pOutputPMMR, &mmrHashesValidated] { if (!this->ValidateMMRHashes(pOutputPMMR)) { mmrHashesValidated = false; }}));
	threads.emplace_back(std::thread([this, pRangeProofPMMR, &mmrHashesValidated] { if (!this->ValidateMMRHashes(pRangeProofPMMR)) { mmrHashesValidated = false; }}));

	for (auto& thread : threads)
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

	std::unique_ptr<BlockSums> pBlockSums = nullptr;
	try
	{
		pBlockSums = std::make_unique<BlockSums>(ValidateKernelSums(txHashSet, blockHeader));
	}
	catch (...)
	{
		LOG_ERROR("Invalid kernel sums");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(40);

	// Validate the rangeproof associated with each unspent output.
	LOG_DEBUG("Validating range proofs");
	LoggerAPI::Flush();
	if (!ValidateRangeProofs(txHashSet, syncStatus))
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

bool TxHashSetValidator::ValidateSizes(TxHashSet& txHashSet, const BlockHeader& blockHeader) const
{
	if (txHashSet.GetKernelMMR()->GetSize() != blockHeader.GetKernelMMRSize())
	{
		LOG_ERROR_F("Kernel size not matching for header ({})", blockHeader);
		return false;
	}

	if (txHashSet.GetOutputPMMR()->GetSize() != blockHeader.GetOutputMMRSize())
	{
		LOG_ERROR_F("Output size not matching for header ({})", blockHeader);
		return false;
	}

	if (txHashSet.GetRangeProofPMMR()->GetSize() != blockHeader.GetOutputMMRSize())
	{
		LOG_ERROR_F("RangeProof size not matching for header ({})", blockHeader);
		return false;
	}

	return true;
}

// TODO: This probably belongs in MMRHashUtil.
bool TxHashSetValidator::ValidateMMRHashes(std::shared_ptr<const MMR> pMMR) const
{
	try
    {
        const uint64_t size = pMMR->GetSize();
        Index mmr_idx = Index::At(0);
        while (mmr_idx < size) {
            if (!mmr_idx.IsLeaf()) {
                const std::unique_ptr<Hash> pParentHash = pMMR->GetHashAt(mmr_idx);
                if (pParentHash != nullptr) {
                    const std::unique_ptr<Hash> pLeftHash = pMMR->GetHashAt(mmr_idx.GetLeftChild());
                    const std::unique_ptr<Hash> pRightHash = pMMR->GetHashAt(mmr_idx.GetRightChild());
                    if (pLeftHash != nullptr && pRightHash != nullptr) {
                        const Hash expectedHash = MMRHashUtil::HashParentWithIndex(*pLeftHash, *pRightHash, mmr_idx.GetPosition());
                        if (*pParentHash != expectedHash) {
                            LOG_ERROR_F("Invalid parent hash at {}", mmr_idx);
                            return false;
                        }
                    }
                }
            }

            ++mmr_idx;
        }
	}
	catch (...)
	{
		return false;
	}

	return true;
}

bool TxHashSetValidator::ValidateKernelHistory(const KernelMMR& kernelMMR, const BlockHeader& blockHeader, SyncStatus& syncStatus) const
{
	const uint64_t totalHeight = blockHeader.GetHeight();
	for (uint64_t height = 0; height <= totalHeight; height++)
	{
		auto pHeader = m_blockChain.GetBlockHeaderByHeight(height, EChainType::CANDIDATE);
		if (pHeader == nullptr)
		{
			LOG_ERROR_F("No header found at height ({})", height);
			return false;
		}
		
		if (kernelMMR.Root(pHeader->GetKernelMMRSize()) != pHeader->GetKernelRoot())
		{
			LOG_ERROR_F("Kernel root not matching for header at height ({})", height);
			return false;
		}

		if (height % 1000 == 0)
		{
			syncStatus.UpdateProcessingStatus((uint8_t)(15 + ((10.0 * height) / totalHeight)));
		}
	}

	return true;
}

BlockSums TxHashSetValidator::ValidateKernelSums(TxHashSet& txHashSet, const BlockHeader& blockHeader) const
{
	// Calculate overage
	const int64_t overage = 0 - (Consensus::REWARD * (1 + blockHeader.GetHeight()));

	// Determine output commitments
	std::shared_ptr<const OutputPMMR> pOutputPMMR = txHashSet.GetOutputPMMR();
	std::vector<Commitment> outputCommitments;
	for (LeafIndex output_idx = LeafIndex::At(0); output_idx < blockHeader.GetNumOutputs(); output_idx++) {
		std::unique_ptr<OutputIdentifier> pOutput = pOutputPMMR->GetAt(output_idx);
		if (pOutput != nullptr) {
			outputCommitments.push_back(pOutput->GetCommitment());
		}
	}

	// Determine kernel excess commitments
	std::shared_ptr<const KernelMMR> pKernelMMR = txHashSet.GetKernelMMR();
	std::vector<Commitment> excessCommitments;
	for (LeafIndex kernel_idx = LeafIndex::At(0); kernel_idx < blockHeader.GetNumKernels(); kernel_idx++) {
		std::unique_ptr<TransactionKernel> pKernel = pKernelMMR->GetKernelAt(kernel_idx);
		if (pKernel != nullptr) {
			excessCommitments.push_back(pKernel->GetExcessCommitment());
		}
	}

	return KernelSumValidator::ValidateKernelSums(
		std::vector<Commitment>(),
		outputCommitments,
		excessCommitments,
		overage,
		blockHeader.GetOffset(),
		std::nullopt
	);
}

bool TxHashSetValidator::ValidateRangeProofs(TxHashSet& txHashSet, SyncStatus& syncStatus) const
{
	std::vector<std::pair<Commitment, RangeProof>> rangeProofs;

	size_t i = 0;
	LOG_INFO("BEGIN");
	const uint64_t outputMMRSize = txHashSet.GetOutputPMMR()->GetSize();
	for (LeafIndex leaf_idx = LeafIndex::At(0); leaf_idx.GetPosition() < outputMMRSize; leaf_idx++)
	{
		std::unique_ptr<OutputIdentifier> pOutput = txHashSet.GetOutputPMMR()->GetAt(leaf_idx);
		if (pOutput != nullptr)
		{
			std::unique_ptr<RangeProof> pRangeProof = txHashSet.GetRangeProofPMMR()->GetAt(leaf_idx);
			if (pRangeProof == nullptr)
			{
				LOG_ERROR_F("No rangeproof found at leaf index ({})", leaf_idx);
				return false;
			}

			rangeProofs.emplace_back(std::make_pair(pOutput->GetCommitment(), *pRangeProof));
			++i;

			if (rangeProofs.size() >= 1000)
			{
				if (!Crypto::VerifyRangeProofs(rangeProofs))
				{
					return false;
				}

				rangeProofs.clear();

				syncStatus.UpdateProcessingStatus((uint8_t)(40 + ((30.0 * leaf_idx.GetPosition()) / outputMMRSize)));
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

	LOG_INFO_F("SUCCESS ({})", i);
	return true;
}

bool TxHashSetValidator::ValidateKernelSignatures(const KernelMMR& kernelMMR, SyncStatus& syncStatus) const
{
	std::vector<TransactionKernel> kernels;

	const uint64_t num_kernels = kernelMMR.GetNumKernels();
	for (LeafIndex leaf_idx = LeafIndex::At(0); leaf_idx < num_kernels; leaf_idx++) {
		std::unique_ptr<TransactionKernel> pKernel = kernelMMR.GetKernelAt(leaf_idx);
		if (pKernel == nullptr) {
			return false;
		}

		kernels.push_back(*pKernel);

		if (kernels.size() >= 2000) {
			if (!KernelSignatureValidator::BatchVerify(kernels)) {
				return false;
			}

			kernels.clear();

			syncStatus.UpdateProcessingStatus((uint8_t)(70 + ((30.0 * leaf_idx.Get()) / num_kernels)));
		}
	}

	if (!KernelSignatureValidator::BatchVerify(kernels)) {
		return false;
	}

	return true;
}