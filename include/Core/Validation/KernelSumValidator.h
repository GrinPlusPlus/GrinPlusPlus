#pragma once

#include <Crypto/Crypto.h>
#include <Crypto/Commitment.h>
#include <Crypto/BlindingFactor.h>
#include <Core/Models/BlockSums.h>
#include <Core/Models/TransactionBody.h>
#include <Common/Util/FunctionalUtil.h>
#include <Infrastructure/Logger.h>
#include <optional>

class KernelSumValidator
{
public:
	// Verify the sum of the kernel excesses equals the sum of the outputs, taking into account both the kernel_offset and overage.
	static std::unique_ptr<BlockSums> ValidateKernelSums(const TransactionBody& transactionBody, const int64_t overage, const BlindingFactor& kernelOffset, const std::optional<BlockSums>& blockSumsOpt)
	{
		// gather the commitments
		auto getInputCommitments = [](TransactionInput& input) -> Commitment { return input.GetCommitment(); };
		std::vector<Commitment> inputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transactionBody.GetInputs(), getInputCommitments);

		auto getOutputCommitments = [](TransactionOutput& output) -> Commitment { return output.GetCommitment(); };
		std::vector<Commitment> outputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transactionBody.GetOutputs(), getOutputCommitments);

		auto getKernelCommitments = [](TransactionKernel& kernel) -> Commitment { return kernel.GetExcessCommitment(); };
		std::vector<Commitment> kernelCommitments = FunctionalUtil::map<std::vector<Commitment>>(transactionBody.GetKernels(), getKernelCommitments);

		return ValidateKernelSums(inputCommitments, outputCommitments, kernelCommitments, overage, kernelOffset, blockSumsOpt);
	}

	static std::unique_ptr<BlockSums> ValidateKernelSums(const std::vector<Commitment>& inputs, const std::vector<Commitment>& outputs, const std::vector<Commitment>& kernels, const int64_t overage, const BlindingFactor& kernelOffset, const std::optional<BlockSums>& blockSumsOpt)
	{
		std::vector<Commitment> inputCommitments = inputs;
		std::vector<Commitment> outputCommitments = outputs;
		if (overage > 0)
		{
			std::unique_ptr<Commitment> pOverCommitment = Crypto::CommitTransparent(overage);
			if (pOverCommitment != nullptr)
			{
				outputCommitments.push_back(*pOverCommitment);
			}
		}
		else if (overage < 0)
		{
			std::unique_ptr<Commitment> pOverCommitment = Crypto::CommitTransparent(0 - overage);
			if (pOverCommitment != nullptr)
			{
				inputCommitments.push_back(*pOverCommitment);
			}
		}

		if (blockSumsOpt.has_value())
		{
			outputCommitments.push_back(blockSumsOpt.value().GetOutputSum());
		}

		// Sum all input|output|overage commitments.
		std::unique_ptr<Commitment> pUTXOSum = Crypto::AddCommitments(outputCommitments, inputCommitments);
		if (pUTXOSum == nullptr)
		{
			LoggerAPI::LogError("ValidationUtil::ValidateKernelSums - Failed to sum input, output, and overage commitments.");
			return std::unique_ptr<BlockSums>(nullptr);
		}

		// Sum the kernel excesses accounting for the kernel offset.
		std::vector<Commitment> kernelCommitments = kernels;
		if (blockSumsOpt.has_value())
		{
			kernelCommitments.push_back(blockSumsOpt.value().GetKernelSum());
		}

		std::unique_ptr<Commitment> pKernelSum = Crypto::AddCommitments(kernelCommitments, std::vector<Commitment>());
		if (pKernelSum == nullptr)
		{
			LoggerAPI::LogError("ValidationUtil::ValidateKernelSums - Failed to sum kernel commitments.");
			return std::unique_ptr<BlockSums>(nullptr);
		}

		std::unique_ptr<Commitment> pKernelSumPlusOffset = AddKernelOffset(*pKernelSum, kernelOffset);
		if (*pUTXOSum != *pKernelSumPlusOffset)
		{
			LoggerAPI::LogError("ValidationUtil::ValidateKernelSums - Failed to validate kernel sums.");
			return std::unique_ptr<BlockSums>(nullptr);
		}

		return std::make_unique<BlockSums>(BlockSums(*pUTXOSum, *pKernelSum));
	}

private:
	static std::unique_ptr<Commitment> AddKernelOffset(const Commitment& kernelSum, const BlindingFactor& totalKernelOffset)
	{
		// Add the commitments along with the commit to zero built from the offset
		std::vector<Commitment> commitments;
		commitments.push_back(kernelSum);

		if (totalKernelOffset.GetBytes() != CBigInteger<32>::ValueOf(0))
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
};