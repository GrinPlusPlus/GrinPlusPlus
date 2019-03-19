#pragma once

#include <Wallet/OutputData.h>
#include <Core/Models/Transaction.h>
#include <Common/Util/FunctionalUtil.h>

class TransactionBuilder
{
public:

	static Transaction BuildTransaction(const std::vector<OutputData>& inputs, const OutputData& changeOutput, const BlindingFactor& transactionOffset, const uint64_t fee, const uint64_t lockHeight)
	{
		auto getInput = [](OutputData& input) -> TransactionInput { return TransactionInput(input.GetOutput().GetFeatures(), Commitment(input.GetOutput().GetCommitment())); };
		std::vector<TransactionInput> transactionInputs = FunctionalUtil::map<std::vector<TransactionInput>>(inputs, getInput);

		std::vector<TransactionOutput> transactionOutputs({ changeOutput.GetOutput() });

		TransactionKernel kernel(EKernelFeatures::HEIGHT_LOCKED, fee, lockHeight, Commitment(CBigInteger<33>::ValueOf(0)), Signature(CBigInteger<64>::ValueOf(0)));
		std::vector<TransactionKernel> kernels({ kernel });

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

	static Transaction AddKernel(const Transaction& transaction, const TransactionKernel& kernel)
	{
		std::vector<TransactionInput> inputs = transaction.GetBody().GetInputs();
		std::vector<TransactionOutput> outputs = transaction.GetBody().GetOutputs();
		std::vector<TransactionKernel> kernels = std::vector<TransactionKernel>({ kernel });//transaction.GetBody().GetKernels();
		TransactionBody transactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
		return Transaction(BlindingFactor(transaction.GetOffset()), std::move(transactionBody));
	}
};