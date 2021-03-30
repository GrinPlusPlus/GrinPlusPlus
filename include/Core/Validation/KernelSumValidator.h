#pragma once

#include <Core/Exceptions/BadDataException.h>
#include <Crypto/Crypto.h>
#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/BlindingFactor.h>
#include <Core/Models/BlockSums.h>
#include <Core/Models/TransactionBody.h>
#include <Common/Util/FunctionalUtil.h>
#include <Common/Logger.h>
#include <optional>

class KernelSumValidator
{
public:
	// Verify the sum of the kernel excesses equals the sum of the outputs, taking into account both the kernel_offset and overage.
	static BlockSums ValidateKernelSums(
		const TransactionBody& transactionBody,
		const int64_t overage,
		const BlindingFactor& kernelOffset,
		const std::optional<BlockSums>& blockSumsOpt)
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

	static BlockSums ValidateKernelSums(
		const std::vector<Commitment>& inputs,
		const std::vector<Commitment>& outputs,
		const std::vector<Commitment>& kernels,
		const int64_t overage,
		const BlindingFactor& kernelOffset,
		const std::optional<BlockSums>& blockSumsOpt)
	{
		std::vector<Commitment> inputCommitments = inputs;
		std::vector<Commitment> outputCommitments = outputs;
		if (overage > 0)
		{
			outputCommitments.push_back(Crypto::CommitTransparent(overage));
		}
		else if (overage < 0)
		{
			inputCommitments.push_back(Crypto::CommitTransparent(0 - overage));
		}

		if (blockSumsOpt.has_value())
		{
			outputCommitments.push_back(blockSumsOpt.value().GetOutputSum());
		}

		// Sum all input|output|overage commitments.
		Commitment utxoSum = Crypto::AddCommitments(outputCommitments, inputCommitments);

		// Sum the kernel excesses accounting for the kernel offset.
		std::vector<Commitment> kernelCommitments = kernels;
		if (blockSumsOpt.has_value())
		{
			kernelCommitments.push_back(blockSumsOpt.value().GetKernelSum());
		}

		Commitment kernelSum = Crypto::AddCommitments(kernelCommitments, std::vector<Commitment>());
		Commitment kernelSumPlusOffset = AddKernelOffset(kernelSum, kernelOffset);
		if (utxoSum != kernelSumPlusOffset) {
			LOG_ERROR_F(
				"UTXO sum {} does not match kernel sum plus offset {}. Kernel sum is {} and offset is {}.",
				utxoSum,
				kernelSumPlusOffset,
				kernelSum,
				kernelOffset.ToHex()
			);
			throw BAD_DATA_EXCEPTION(EBanReason::BadBlock, "UTXO sum does not match kernel sum plus offset");
		}

		return BlockSums(utxoSum, kernelSum);
	}

private:
	static Commitment AddKernelOffset(const Commitment& kernelSum, const BlindingFactor& totalKernelOffset)
	{
		if (totalKernelOffset == CBigInteger<32>::ValueOf(0)) {
			return kernelSum;
		}

		// Add the commitments along with the commit to zero built from the offset
		Commitment offset_commit = Crypto::CommitBlinded((uint64_t)0, totalKernelOffset);
		return Crypto::AddCommitments(
			std::vector<Commitment>{ kernelSum, std::move(offset_commit) },
			std::vector<Commitment>()
		);
	}
};