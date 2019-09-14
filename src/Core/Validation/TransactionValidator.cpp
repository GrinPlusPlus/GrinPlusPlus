#include <Core/Validation/TransactionValidator.h>
#include <Core/Validation/TransactionBodyValidator.h>

#include <Crypto/Crypto.h>
#include <Common/Util/HexUtil.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/FunctionalUtil.h>

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
bool TransactionValidator::ValidateKernelSums(const Transaction& transaction)
{
	// Calculate overage
	int64_t overage = 0;
	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		overage += (int64_t)kernel.GetFee();
	}

	return KernelSumValidator::ValidateKernelSums(transaction.GetBody(), overage, transaction.GetOffset(), std::nullopt) != nullptr;
}