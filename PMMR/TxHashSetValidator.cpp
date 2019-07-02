#include "TxHashSetValidator.h"
#include "TxHashSetImpl.h"
#include "KernelMMR.h"
#include "Common/MMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Consensus/Common.h>
#include <Common/Util/HexUtil.h>
#include <Infrastructure/Logger.h>
#include <BlockChain/BlockChainServer.h>
#include <thread>

TxHashSetValidator::TxHashSetValidator(const IBlockChainServer& blockChainServer)
	: m_blockChainServer(blockChainServer)
{

}

std::unique_ptr<BlockSums> TxHashSetValidator::Validate(TxHashSet& txHashSet, const BlockHeader& blockHeader, SyncStatus& syncStatus) const
{
	const KernelMMR& kernelMMR = *txHashSet.GetKernelMMR();
	const OutputPMMR& outputPMMR = *txHashSet.GetOutputPMMR();
	const RangeProofPMMR& rangeProofPMMR = *txHashSet.GetRangeProofPMMR();

	// Validate size of each MMR matches blockHeader
	if (!ValidateSizes(txHashSet, blockHeader))
	{
		LoggerAPI::LogError("TxHashSetValidator::Validate - Invalid MMR size.");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(5);

	// Validate MMR hashes in parallel
	std::vector<std::thread> threads;
	std::atomic_bool mmrHashesValidated = true;
	threads.emplace_back(std::thread([this, &kernelMMR, &mmrHashesValidated] { if (!this->ValidateMMRHashes(kernelMMR)) { mmrHashesValidated = false; }}));
	threads.emplace_back(std::thread([this, &outputPMMR, &mmrHashesValidated] { if (!this->ValidateMMRHashes(outputPMMR)) { mmrHashesValidated = false; }}));
	threads.emplace_back(std::thread([this, &rangeProofPMMR, &mmrHashesValidated] { if (!this->ValidateMMRHashes(rangeProofPMMR)) { mmrHashesValidated = false; }}));

	for (auto& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	if (!mmrHashesValidated)
	{
		LoggerAPI::LogError("TxHashSetValidator::Validate - Invalid MMR hashes.");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(10);

	// Validate root for each MMR matches blockHeader
	if (!txHashSet.ValidateRoots(blockHeader))
	{
		LoggerAPI::LogError("TxHashSetValidator::Validate - Invalid MMR roots.");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(15);

	// Validate the full kernel history (kernel MMR root for every block header).
	LoggerAPI::LogDebug("TxHashSetValidator::Validate - Validating kernel history.");
	if (!ValidateKernelHistory(*txHashSet.GetKernelMMR(), blockHeader))
	{
		LoggerAPI::LogError("TxHashSetValidator::Validate - Invalid kernel history.");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(25);

	// Validate kernel sums
	LoggerAPI::LogDebug("TxHashSetValidator::Validate - Validating kernel sums.");
	std::unique_ptr<BlockSums> pBlockSums = ValidateKernelSums(txHashSet, blockHeader);
	if (pBlockSums == nullptr)
	{
		LoggerAPI::LogError("TxHashSetValidator::Validate - Invalid kernel sums.");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(40);

	// Validate the rangeproof associated with each unspent output.
	LoggerAPI::LogDebug("TxHashSetValidator::Validate - Validating range proofs.");
	if (!ValidateRangeProofs(txHashSet, blockHeader))
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateRangeProofs - Failed to verify rangeproofs.");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(70);

	// Validate kernel signatures
	LoggerAPI::LogDebug("TxHashSetValidator::Validate - Validating kernel signatures.");
	if (!ValidateKernelSignatures(*txHashSet.GetKernelMMR()))
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateKernelSignatures - Failed to verify kernel signatures.");
		return std::unique_ptr<BlockSums>(nullptr);
	}

	syncStatus.UpdateProcessingStatus(100);

	return pBlockSums;
}

bool TxHashSetValidator::ValidateSizes(TxHashSet& txHashSet, const BlockHeader& blockHeader) const
{
	if (txHashSet.GetKernelMMR()->GetSize() != blockHeader.GetKernelMMRSize())
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateSizes - Kernel size not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	if (txHashSet.GetOutputPMMR()->GetSize() != blockHeader.GetOutputMMRSize())
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateSizes - Output size not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	if (txHashSet.GetRangeProofPMMR()->GetSize() != blockHeader.GetOutputMMRSize())
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateSizes - RangeProof size not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	return true;
}

// TODO: This probably belongs in MMRHashUtil.
bool TxHashSetValidator::ValidateMMRHashes(const MMR& mmr) const
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
						LoggerAPI::LogError("TxHashSetValidator::ValidateMMRHashes - Invalid parent hash at index " + std::to_string(i));
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool TxHashSetValidator::ValidateKernelHistory(const KernelMMR& kernelMMR, const BlockHeader& blockHeader) const
{
	for (uint64_t height = 0; height <= blockHeader.GetHeight(); height++)
	{
		std::unique_ptr<BlockHeader> pHeader = m_blockChainServer.GetBlockHeaderByHeight(height, EChainType::CANDIDATE);
		if (pHeader == nullptr)
		{
			LoggerAPI::LogError("TxHashSetValidator::ValidateKernelHistory - No header found at height " + std::to_string(height));
			return false;
		}
		
		if (kernelMMR.Root(pHeader->GetKernelMMRSize()) != pHeader->GetKernelRoot())
		{
			LoggerAPI::LogError("TxHashSetValidator::ValidateKernelHistory - Kernel root not matching for header at height " + std::to_string(height));
			return false;
		}
	}

	return true;
}

std::unique_ptr<BlockSums> TxHashSetValidator::ValidateKernelSums(TxHashSet& txHashSet, const BlockHeader& blockHeader) const
{
	// Calculate overage
	const int64_t overage = 0 - (Consensus::REWARD * (1 + blockHeader.GetHeight()));

	// Determine output commitments
	OutputPMMR* pOutputPMMR = txHashSet.GetOutputPMMR();
	std::vector<Commitment> outputCommitments;
	for (uint64_t i = 0; i < blockHeader.GetOutputMMRSize(); i++)
	{
		std::unique_ptr<OutputIdentifier> pOutput = pOutputPMMR->GetOutputAt(i);
		if (pOutput != nullptr)
		{
			outputCommitments.push_back(pOutput->GetCommitment());
		}
	}

	// Determine kernel excess commitments
	KernelMMR* pKernelMMR = txHashSet.GetKernelMMR();
	std::vector<Commitment> excessCommitments;
	for (uint64_t i = 0; i < blockHeader.GetKernelMMRSize(); i++)
	{
		std::unique_ptr<TransactionKernel> pKernel = pKernelMMR->GetKernelAt(i);
		if (pKernel != nullptr)
		{
			excessCommitments.push_back(pKernel->GetExcessCommitment());
		}
	}

	return KernelSumValidator::ValidateKernelSums(std::vector<Commitment>(), outputCommitments, excessCommitments, overage, blockHeader.GetTotalKernelOffset(), std::nullopt);
}

bool TxHashSetValidator::ValidateRangeProofs(TxHashSet& txHashSet, const BlockHeader& blockHeader) const
{
	std::vector<std::pair<Commitment, RangeProof>> rangeProofs;

	size_t i = 0;
	LoggerAPI::LogInfo("TxHashSetValidator::ValidateRangeProofs - BEGIN");
	for (uint64_t mmrIndex = 0; mmrIndex < txHashSet.GetOutputPMMR()->GetSize(); mmrIndex++)
	{
		std::unique_ptr<OutputIdentifier> pOutput = txHashSet.GetOutputPMMR()->GetOutputAt(mmrIndex);
		if (pOutput != nullptr)
		{
			std::unique_ptr<RangeProof> pRangeProof = txHashSet.GetRangeProofPMMR()->GetRangeProofAt(mmrIndex);
			if (pRangeProof == nullptr)
			{
				LoggerAPI::LogError("TxHashSetValidator::ValidateRangeProofs - No rangeproof found at mmr index " + std::to_string(mmrIndex));
				return false;
			}

			rangeProofs.emplace_back(std::make_pair<Commitment, RangeProof>(Commitment(pOutput->GetCommitment()), RangeProof(*pRangeProof)));
			++i;

			if (rangeProofs.size() >= 1000)
			{
				if (!Crypto::VerifyRangeProofs(rangeProofs))
				{
					return false;
				}

				rangeProofs.clear();
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

	LoggerAPI::LogInfo("TxHashSetValidator::ValidateRangeProofs - SUCCESS " + std::to_string(i));
	return true;
}

bool TxHashSetValidator::ValidateKernelSignatures(const KernelMMR& kernelMMR) const
{
	std::vector<TransactionKernel> kernels;

	const uint64_t mmrSize = kernelMMR.GetSize();
	const uint64_t numKernels = MMRUtil::GetNumLeaves(mmrSize);
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