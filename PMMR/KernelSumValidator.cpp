#include "KernelSumValidator.h"
#include "Common/MMRUtil.h"

#include <HexUtil.h>
#include <Crypto.h>
#include <Consensus/Common.h>
#include <Infrastructure/Logger.h>

// TODO: Refactor to Use Core/Validation/KernelSumValidator.h
bool KernelSumValidator::ValidateKernelSums(TxHashSet& txHashSet, const BlockHeader& blockHeader, const bool genesisHasReward, Commitment& outputSumOut, Commitment& kernelSumOut) const
{
	// Calculate overage
	const int64_t genesisReward = genesisHasReward ? 1 : 0;
	const uint64_t overage = (genesisReward + blockHeader.GetHeight()) * Consensus::REWARD;

	// Sum all outputs & overage commitments.
	std::unique_ptr<Commitment> pUtxoSum = AddCommitments(txHashSet, overage, blockHeader.GetOutputMMRSize());
	if (pUtxoSum == nullptr)
	{
		LoggerAPI::LogError("KernelSumValidator::ValidateKernelSums - Failed to add commitments for block " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	// Sum the kernel excesses
	std::unique_ptr<Commitment> pKernelSum = AddKernelExcesses(txHashSet, blockHeader.GetKernelMMRSize());
	if (pKernelSum == nullptr)
	{
		LoggerAPI::LogError("KernelSumValidator::ValidateKernelSums - Failed to add excess commitments.");
		return false;
	}

	// Accounting for the kernel offset.
	std::unique_ptr<Commitment> pKernelSumPlusOffset = AddKernelOffset(*pKernelSum, blockHeader.GetTotalKernelOffset());
	if (pKernelSumPlusOffset == nullptr)
	{
		LoggerAPI::LogError("KernelSumValidator::ValidateKernelSums - Failed to add kernel excesses for block " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	if (*pUtxoSum != *pKernelSumPlusOffset)
	{
		LoggerAPI::LogError("KernelSumValidator::ValidateKernelSums - Kernel sum with offset not matching utxo sum for block " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	// Populate the output sum and kernel sum
	outputSumOut = *pUtxoSum;
	kernelSumOut = *pKernelSum;

	return true;
}

std::unique_ptr<Commitment> KernelSumValidator::AddCommitments(TxHashSet& txHashSet, const uint64_t overage, const uint64_t outputMMRSize) const
{
	// Determine over-commitment
	std::unique_ptr<Commitment> pOverageCommitment = Crypto::CommitTransparent(overage);
	if (pOverageCommitment == nullptr)
	{
		LoggerAPI::LogError("KernelSumValidator::AddCommitments - Failed to commit to overage: " + std::to_string(overage));
		return std::unique_ptr<Commitment>(nullptr);
	}

	const std::vector<Commitment> overCommitment({ *pOverageCommitment });

	// Determine output commitments
	OutputPMMR* pOutputPMMR = txHashSet.GetOutputPMMR();
	std::vector<Commitment> outputCommitments;
	for (uint64_t i = 0; i < outputMMRSize; i++)
	{
		std::unique_ptr<OutputIdentifier> pOutput = pOutputPMMR->GetOutputAt(i);
		if (pOutput != nullptr)
		{
			outputCommitments.push_back(pOutput->GetCommitment());
		}
	}

	return Crypto::AddCommitments(outputCommitments, overCommitment);
}

std::unique_ptr<Commitment> KernelSumValidator::AddKernelExcesses(TxHashSet& txHashSet, const uint64_t kernelMMRSize) const
{
	// Determine kernel excess commitments
	KernelMMR* pKernelMMR = txHashSet.GetKernelMMR();
	std::vector<Commitment> excessCommitments;
	for (uint64_t i = 0; i < kernelMMRSize; i++)
	{
		std::unique_ptr<TransactionKernel> pKernel = pKernelMMR->GetKernelAt(i);
		if (pKernel != nullptr)
		{
			excessCommitments.push_back(pKernel->GetExcessCommitment());
		}
	}

	// Add the kernel excess commitments
	return Crypto::AddCommitments(excessCommitments, std::vector<Commitment>());
}

std::unique_ptr<Commitment> KernelSumValidator::AddKernelOffset(const Commitment& kernelSum, const BlindingFactor& totalKernelOffset) const
{
	// Add the commitments along with the commit to zero built from the offset
	std::vector<Commitment> commitments;
	commitments.push_back(kernelSum);

	if (totalKernelOffset.GetBlindingFactorBytes() != CBigInteger<32>::ValueOf(0))
	{
		std::unique_ptr<Commitment> pOffsetCommitment = Crypto::CommitBlinded((uint64_t)0, totalKernelOffset);
		if (pOffsetCommitment == nullptr)
		{
			LoggerAPI::LogError("KernelSumValidator::AddKernelOffset - Failed to commit kernel offset.");
			return std::unique_ptr<Commitment>(nullptr);
		}

		commitments.push_back(*pOffsetCommitment);
	}

	return Crypto::AddCommitments(commitments, std::vector<Commitment>());
}