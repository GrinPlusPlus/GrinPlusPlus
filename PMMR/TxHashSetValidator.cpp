#include "TxHashSetValidator.h"
#include "TxHashSetImpl.h"
#include "KernelMMR.h"
#include "Common/MMR.h"
#include "Common/MMRUtil.h"
#include "KernelSumValidator.h"
#include "KernelSignatureValidator.h"

#include <HexUtil.h>
#include <Infrastructure/Logger.h>
#include <BlockChainServer.h>
#include <async++.h>

TxHashSetValidator::TxHashSetValidator(const IBlockChainServer& blockChainServer)
	: m_blockChainServer(blockChainServer)
{

}

// TODO: Where do we validate the data in MMR actually hashes to HashFile's hash?
TxHashSetValidationResult TxHashSetValidator::Validate(TxHashSet& txHashSet, const BlockHeader& blockHeader, Commitment& outputSumOut, Commitment& kernelSumOut) const
{
	const KernelMMR& kernelMMR = *txHashSet.GetKernelMMR();
	const OutputPMMR& outputPMMR = *txHashSet.GetOutputPMMR();
	const RangeProofPMMR& rangeProofPMMR = *txHashSet.GetRangeProofPMMR();

	// Validate size of each MMR matches blockHeader
	if (!ValidateSizes(txHashSet, blockHeader))
	{
		return TxHashSetValidationResult::Fail();
	}

	// Validate MMR hashes in parallel
	async::task<bool> kernelTask = async::spawn([this, &kernelMMR] { return this->ValidateMMRHashes(kernelMMR); });
	async::task<bool> outputTask = async::spawn([this, &outputPMMR] { return this->ValidateMMRHashes(outputPMMR); });
	async::task<bool> rangeProofTask = async::spawn([this, &rangeProofPMMR] { return this->ValidateMMRHashes(rangeProofPMMR); });

	const bool mmrHashesValidated = async::when_all(kernelTask, outputTask, rangeProofTask).then(
		[](std::tuple<async::task<bool>, async::task<bool>, async::task<bool>> results) -> bool {
		return std::get<0>(results).get() && std::get<1>(results).get() && std::get<2>(results).get();
	}).get();
	
	if (!mmrHashesValidated)
	{
		return TxHashSetValidationResult::Fail();
	}

	// Validate root for each MMR matches blockHeader
	if (!ValidateRoots(txHashSet, blockHeader))
	{
		return TxHashSetValidationResult::Fail();
	}

	// Validate the full kernel history (kernel MMR root for every block header).
	if (!ValidateKernelHistory(*txHashSet.GetKernelMMR(), blockHeader))
	{
		return TxHashSetValidationResult::Fail();
	}

	// Validate kernel sums
	const std::unique_ptr<BlockHeader> pGenesisHeader = m_blockChainServer.GetBlockHeaderByHeight(0, EChainType::CANDIDATE);
	const bool genesisHasReward = pGenesisHeader->GetKernelMMRSize() > 0;
	Commitment outputSum(CBigInteger<33>::ValueOf(0));
	Commitment kernelSum(CBigInteger<33>::ValueOf(0));
	if (!KernelSumValidator().ValidateKernelSums(txHashSet, blockHeader, genesisHasReward, outputSum, kernelSum))
	{
		// TODO: return TxHashSetValidationResult::Fail();
	}

	// Validate the rangeproof associated with each unspent output.

	// Validate kernel signatures
	if (!KernelSignatureValidator().ValidateKernelSignatures(*txHashSet.GetKernelMMR()))
	{
		return TxHashSetValidationResult::Fail();
	}


	return TxHashSetValidationResult(true, std::move(outputSum), std::move(kernelSum));
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
					const Hash expectedHash = MMRUtil::HashParentWithIndex(*pLeftHash, *pRightHash, i);
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

bool TxHashSetValidator::ValidateRoots(TxHashSet& txHashSet, const BlockHeader& blockHeader) const
{
	if (txHashSet.GetKernelMMR()->Root(blockHeader.GetKernelMMRSize()) != blockHeader.GetKernelRoot())
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateRoots - Kernel root not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	if (txHashSet.GetOutputPMMR()->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetOutputRoot())
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateRoots - Output root not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	if (txHashSet.GetRangeProofPMMR()->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetRangeProofRoot())
	{
		LoggerAPI::LogError("TxHashSetValidator::ValidateRoots - RangeProof root not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
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