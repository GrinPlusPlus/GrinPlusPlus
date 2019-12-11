#pragma once

#include <Common/Util/FunctionalUtil.h>
#include <Crypto/Commitment.h>
#include <Crypto/Crypto.h>
#include <Wallet/Models/Slate/Slate.h>

class SlateUtil
{
public:
	static Commitment CalculateFinalExcess(const Slate& slate)
	{
		const Transaction& transaction = slate.GetTransaction();
		const TransactionKernel& kernel = slate.GetTransaction().GetBody().GetKernels().front();

		// Build the final excess based on final tx and offset
		std::vector<Commitment> inputCommitments;
		std::transform(
			transaction.GetInputs().cbegin(),
			transaction.GetInputs().cend(),
			std::back_inserter(inputCommitments),
			[](const TransactionInput& input) -> Commitment { return input.GetCommitment(); }
		);
		inputCommitments.emplace_back(Crypto::CommitBlinded(0, transaction.GetOffset()));

		std::vector<Commitment> outputCommitments;
		std::transform(
			transaction.GetOutputs().cbegin(),
			transaction.GetOutputs().cend(),
			std::back_inserter(outputCommitments),
			[](const TransactionOutput& output) -> Commitment { return output.GetCommitment(); }
		);
		outputCommitments.emplace_back(Crypto::CommitTransparent(kernel.GetFee()));

		return Crypto::AddCommitments(outputCommitments, inputCommitments);
	}
};