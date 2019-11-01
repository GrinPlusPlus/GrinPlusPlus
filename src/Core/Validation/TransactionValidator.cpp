#include <Core/Validation/TransactionValidator.h>
#include <Core/Validation/TransactionBodyValidator.h>

#include <Crypto/Crypto.h>
#include <Common/Util/HexUtil.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/FunctionalUtil.h>
#include <numeric>

// See: https://github.com/mimblewimble/docs/wiki/Validation-logic
bool TransactionValidator::ValidateTransaction(const Transaction& transaction)
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

bool TransactionValidator::ValidateFeatures(const TransactionBody& transactionBody)
{
	// Verify no output features.
	const bool hasCoinbaseOutputs = std::any_of(
		transactionBody.GetOutputs().cbegin(),
		transactionBody.GetOutputs().cend(),
		[](const TransactionOutput& output) { return output.IsCoinbase(); }
	);

	// Verify no kernel features.
	const bool hasCoinbaseKernels = std::any_of(
		transactionBody.GetKernels().cbegin(),
		transactionBody.GetKernels().cend(),
		[](const TransactionKernel& kernel) { return kernel.IsCoinbase(); }
	);

	return !hasCoinbaseOutputs && !hasCoinbaseKernels;
}

// Verify the sum of the kernel excesses equals the sum of the outputs, taking into account both the kernel_offset and overage.
bool TransactionValidator::ValidateKernelSums(const Transaction& transaction)
{
	// Calculate overage
	const std::vector<TransactionKernel>& blockKernels = transaction.GetBody().GetKernels();
	const int64_t overage = std::accumulate(
		blockKernels.cbegin(),
		blockKernels.cend(),
		(int64_t)0,
		[](int64_t overage, const TransactionKernel& kernel) { return overage + (int64_t)kernel.GetFee(); }
	);

	return KernelSumValidator::ValidateKernelSums(transaction.GetBody(), overage, transaction.GetOffset(), std::nullopt) != nullptr;
}