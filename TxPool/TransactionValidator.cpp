#include "TransactionValidator.h"
#include "TransactionBodyValidator.h"

// See: https://github.com/mimblewimble/docs/wiki/Validation-logic
bool TransactionValidator::ValidateTransaction(const Transaction& transaction) const
{
	// Validate the "transaction body"
	if (!TransactionBodyValidator().ValidateTransactionBody(transaction.GetBody(), false))
	{
		return false;
	}

	// Verify no output or kernel include invalid features (coinbase)
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

	// TODO: Implement
	// Sum all input|output|overage commitments.
	//let utxo_sum = self.sum_commitments(overage) ? ;

	//// Sum the kernel excesses accounting for the kernel offset.
	//let(kernel_sum, kernel_sum_plus_offset) = self.sum_kernel_excesses(&kernel_offset) ? ;

	//if utxo_sum != kernel_sum_plus_offset{
	//	return Err(Error::KernelSumMismatch);
	//}


	return true;
}