#pragma once

#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Core/Models/Transaction.h>
#include <Common/Util/FunctionalUtil.h>

class TransactionBuilder
{
public:
	static Transaction BuildTransaction(
		const std::vector<TransactionInput>& inputs,
		const std::vector<TransactionOutput>& outputs,
		const TransactionKernel& kernel,
		const BlindingFactor& transactionOffset)
	{
		std::vector<TransactionInput> transactionInputs = inputs;
		std::vector<TransactionOutput> transactionOutputs = outputs;
		std::vector<TransactionKernel> kernels({ kernel });

		std::sort(transactionInputs.begin(), transactionInputs.end(), SortInputsByHash);
		std::sort(transactionOutputs.begin(), transactionOutputs.end(), SortOutputsByHash);
		std::sort(kernels.begin(), kernels.end(), SortKernelsByHash);

		TransactionBody body(
			std::move(transactionInputs),
			std::move(transactionOutputs),
			std::move(kernels)
		);

		return Transaction(BlindingFactor(transactionOffset), std::move(body));
	}
};