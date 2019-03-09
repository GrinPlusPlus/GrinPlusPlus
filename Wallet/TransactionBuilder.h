#pragma once

#include "WalletCoin.h"

#include <Core/Models/Transaction.h>
#include <Common/Util/FunctionalUtil.h>

class TransactionBuilder
{
public:

	static Transaction BuildTransaction(const std::vector<WalletCoin>& inputs, const WalletCoin& changeOutput, const BlindingFactor& transactionOffset, const uint64_t fee, const uint64_t lockHeight)
	{
		auto getInput = [](WalletCoin& input) -> TransactionInput { return TransactionInput(input.GetOutputData().GetOutput().GetFeatures(), Commitment(input.GetOutputData().GetOutput().GetCommitment())); };
		std::vector<TransactionInput> transactionInputs = FunctionalUtil::map<std::vector<TransactionInput>>(inputs, getInput);

		std::vector<TransactionOutput> transactionOutputs({ changeOutput.GetOutputData().GetOutput() });

		TransactionKernel kernel(EKernelFeatures::HEIGHT_LOCKED, fee, lockHeight, Commitment(CBigInteger<33>::ValueOf(0)), Signature(CBigInteger<64>::ValueOf(0)));
		std::vector<TransactionKernel> kernels({ kernel });

		return Transaction(BlindingFactor(transactionOffset), TransactionBody(std::move(transactionInputs), std::move(transactionOutputs), std::move(kernels)));
	}

	static Transaction AddOutput(const Transaction& transaction, const TransactionOutput& output)
	{
		std::vector<TransactionInput> inputs = transaction.GetBody().GetInputs();
		std::vector<TransactionOutput> outputs = transaction.GetBody().GetOutputs();
		outputs.push_back(output);
		std::vector<TransactionKernel> kernels = transaction.GetBody().GetKernels();
		TransactionBody transactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
		return Transaction(BlindingFactor(transaction.GetOffset()), std::move(transactionBody));
	}

	static Transaction AddKernel(const Transaction& transaction, const TransactionKernel& kernel)
	{
		std::vector<TransactionInput> inputs = transaction.GetBody().GetInputs();
		std::vector<TransactionOutput> outputs = transaction.GetBody().GetOutputs();
		std::vector<TransactionKernel> kernels = transaction.GetBody().GetKernels();
		kernels.push_back(kernel);
		TransactionBody transactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
		return Transaction(BlindingFactor(transaction.GetOffset()), std::move(transactionBody));
	}
};