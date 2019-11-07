#include <Core/Validation/TransactionValidator.h>
#include <Core/Validation/TransactionBodyValidator.h>

#include <Crypto/Crypto.h>
#include <Common/Util/HexUtil.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/FunctionalUtil.h>
#include <numeric>

// See: https://github.com/mimblewimble/docs/wiki/Validation-logic
void TransactionValidator::Validate(const Transaction& transaction) const
{
	// Validate the "transaction body"
	TransactionBodyValidator().Validate(transaction.GetBody(), false);

	// Verify no output or kernel includes invalid features (coinbase)
	ValidateFeatures(transaction.GetBody());

	// Verify the big "sum": all inputs plus reward+fee, all output commitments, all kernels plus the kernel excess
	ValidateKernelSums(transaction);
}

void TransactionValidator::ValidateFeatures(const TransactionBody& transactionBody) const
{
	// Verify no output features.
	const bool hasCoinbaseOutputs = std::any_of(
		transactionBody.GetOutputs().cbegin(),
		transactionBody.GetOutputs().cend(),
		[](const TransactionOutput& output) { return output.IsCoinbase(); }
	);
	if (hasCoinbaseOutputs)
	{
		throw BAD_DATA_EXCEPTION("Transaction contains coinbase outputs.");
	}

	// Verify no kernel features.
	const bool hasCoinbaseKernels = std::any_of(
		transactionBody.GetKernels().cbegin(),
		transactionBody.GetKernels().cend(),
		[](const TransactionKernel& kernel) { return kernel.IsCoinbase(); }
	);
	if (hasCoinbaseKernels)
	{
		throw BAD_DATA_EXCEPTION("Transaction contains coinbase kernels.");
	}
}

// Verify the sum of the kernel excesses equals the sum of the outputs, taking into account both the kernel_offset and overage.
void TransactionValidator::ValidateKernelSums(const Transaction& transaction) const
{
	// Calculate overage
	const std::vector<TransactionKernel>& blockKernels = transaction.GetBody().GetKernels();
	const int64_t overage = std::accumulate(
		blockKernels.cbegin(),
		blockKernels.cend(),
		(int64_t)0,
		[](int64_t overage, const TransactionKernel& kernel) { return overage + (int64_t)kernel.GetFee(); }
	);
	
	try
	{
		KernelSumValidator::ValidateKernelSums(transaction.GetBody(), overage, transaction.GetOffset(), std::nullopt);
	}
	catch (std::exception& e)
	{
		LOG_WARNING_F("Kernel sum validation failed with error: %s", e.what());
		throw BAD_DATA_EXCEPTION("Kernel sum validation failed.");
	}
}