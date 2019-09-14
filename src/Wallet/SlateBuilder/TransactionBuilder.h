#pragma once

#include <Wallet/OutputData.h>
#include <Core/Models/Transaction.h>
#include <Common/Util/FunctionalUtil.h>

class TransactionBuilder
{
public:

	static Transaction BuildTransaction(const std::vector<OutputData>& inputs, const std::vector<OutputData>& changeOutputs, const BlindingFactor& transactionOffset, const uint64_t fee, const uint64_t lockHeight)
	{
		auto getInput = [](OutputData& input) -> TransactionInput { return TransactionInput(input.GetOutput().GetFeatures(), Commitment(input.GetOutput().GetCommitment())); };
		std::vector<TransactionInput> transactionInputs = FunctionalUtil::map<std::vector<TransactionInput>>(inputs, getInput);

		auto getOutput = [](OutputData& output) -> TransactionOutput { return output.GetOutput(); };
		std::vector<TransactionOutput> transactionOutputs = FunctionalUtil::map<std::vector<TransactionOutput>>(changeOutputs, getOutput);

		const EKernelFeatures kernelFeatures = (lockHeight == 0) ? EKernelFeatures::DEFAULT_KERNEL : EKernelFeatures::HEIGHT_LOCKED;
		TransactionKernel kernel(kernelFeatures, fee, lockHeight, Commitment(CBigInteger<33>::ValueOf(0)), Signature(CBigInteger<64>::ValueOf(0)));
		std::vector<TransactionKernel> kernels({ std::move(kernel) });

		std::sort(transactionInputs.begin(), transactionInputs.end(), SortInputsByHash);
		std::sort(transactionOutputs.begin(), transactionOutputs.end(), SortOutputsByHash);
		std::sort(kernels.begin(), kernels.end(), SortKernelsByHash);

		return Transaction(BlindingFactor(transactionOffset), TransactionBody(std::move(transactionInputs), std::move(transactionOutputs), std::move(kernels)));
	}

	static Transaction AddOutput(const Transaction& transaction, const TransactionOutput& output)
	{
		std::vector<TransactionInput> inputs = transaction.GetBody().GetInputs();
		std::vector<TransactionKernel> kernels = transaction.GetBody().GetKernels();
		std::vector<TransactionOutput> outputs = transaction.GetBody().GetOutputs();
		outputs.push_back(output);
		std::sort(outputs.begin(), outputs.end(), SortOutputsByHash);

		TransactionBody transactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
		return Transaction(BlindingFactor(transaction.GetOffset()), std::move(transactionBody));
	}

	static Transaction ReplaceKernel(const Transaction& transaction, const TransactionKernel& kernel)
	{
		std::vector<TransactionInput> inputs = transaction.GetBody().GetInputs();
		std::vector<TransactionOutput> outputs = transaction.GetBody().GetOutputs();
		std::vector<TransactionKernel> kernels = std::vector<TransactionKernel>({ kernel });
		TransactionBody transactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
		return Transaction(BlindingFactor(transaction.GetOffset()), std::move(transactionBody));
	}
};