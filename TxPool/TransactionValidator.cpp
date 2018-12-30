#include "TransactionValidator.h"
#include "TransactionBodyValidator.h"

#include <Crypto.h>
#include <HexUtil.h>
#include <Infrastructure/Logger.h>
#include <Common/FunctionalUtil.h>

// See: https://github.com/mimblewimble/docs/wiki/Validation-logic
bool TransactionValidator::ValidateTransaction(const Transaction& transaction) const
{
	// Validate the "transaction body"
	if (!TransactionBodyValidator().ValidateTransactionBody(transaction.GetBody(), false))
	{
		return false;
	}

	// Verify no output or kernel includes invalid features (coinbase)
	if (!ValidateFeatures(transaction.GetBody()))
	{
		return false;
	}

	// Verify the big "sum": all inputs plus reward+fee, all output commitments, all kernels plus the kernel excess
	if (!ValidateKernelSums(transaction))
	{
		return false;
	}

	return true;
}

bool TransactionValidator::ValidateFeatures(const TransactionBody& transactionBody) const
{
	// Validate output features.
	for (const TransactionOutput& output : transactionBody.GetOutputs())
	{
		if ((output.GetFeatures() & EOutputFeatures::COINBASE_OUTPUT) == EOutputFeatures::COINBASE_OUTPUT)
		{
			return false;
		}
	}

	// Validate kernel features.
	for (const TransactionKernel& kernel : transactionBody.GetKernels())
	{
		if ((kernel.GetFeatures() & EKernelFeatures::COINBASE_KERNEL) == EKernelFeatures::COINBASE_KERNEL)
		{
			return false;
		}
	}

	return true;
}

// Verify the sum of the kernel excesses equals the sum of the outputs, taking into account both the kernel_offset and overage.
bool TransactionValidator::ValidateKernelSums(const Transaction& transaction) const
{
	// Calculate overage
	uint64_t overage = 0;
	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		overage += kernel.GetFee();
	}

	// gather the commitments
	auto getInputCommitments = [](TransactionInput& input) -> Commitment { return input.GetCommitment(); };
	std::vector<Commitment> inputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetInputs(), getInputCommitments);

	auto getOutputCommitments = [](TransactionOutput& output) -> Commitment { return output.GetCommitment(); };
	std::vector<Commitment> outputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetOutputs(), getOutputCommitments);

	// Add the overage as output commitment
	if (overage != 0)
	{
		std::unique_ptr<Commitment> pOverCommitment = Crypto::CommitTransparent(overage);
		if (nullptr != pOverCommitment)
		{
			outputCommitments.push_back(*pOverCommitment);
		}
	}

	// Sum all input|output|overage commitments.
	std::unique_ptr<Commitment> pUTXOSum = Crypto::AddCommitments(outputCommitments, inputCommitments);

	// Sum the kernel excesses accounting for the kernel offset.
	auto getKernelCommitments = [](TransactionKernel& kernel) -> Commitment { return kernel.GetExcessCommitment(); };
	std::vector<Commitment> kernelCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetKernels(), getKernelCommitments);

	if (transaction.GetOffset() != BlindingFactor(CBigInteger<32>::ValueOf(0)))
	{
		std::unique_ptr<Commitment> pOffsetCommitment = Crypto::CommitBlinded((uint64_t)0, transaction.GetOffset());
		kernelCommitments.push_back(*pOffsetCommitment);
	}

	std::unique_ptr<Commitment> pKernelSumPlusOffset = Crypto::AddCommitments(kernelCommitments, std::vector<Commitment>());

	if (*pUTXOSum != *pKernelSumPlusOffset)
	{
		LoggerAPI::LogError("TransactionValidator::ValidateKernelSums - Failed to validate kernel sums for transaction " + HexUtil::ConvertHash(transaction.GetHash()));
		return false;
	}

	return true;
}