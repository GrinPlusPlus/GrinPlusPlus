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
		return CalculateExcess(slate.sigs);
		//std::vector<TransactionInput> inputs = slate.GetInputs();
		//std::vector<TransactionOutput> outputs = slate.GetOutputs();

		//std::vector<Commitment> inputCommitments;
		//std::transform(
		//	inputs.cbegin(), inputs.cend(),
		//	std::back_inserter(inputCommitments),
		//	[](const TransactionInput& input) -> Commitment { return input.GetCommitment(); }
		//);
		//inputCommitments.emplace_back(Crypto::CommitBlinded(0, slate.offset));

		//std::vector<Commitment> outputCommitments;
		//std::transform(
		//	outputs.cbegin(), outputs.cend(),
		//	std::back_inserter(outputCommitments),
		//	[](const TransactionOutput& output) -> Commitment { return output.GetCommitment(); }
		//);
		//outputCommitments.emplace_back(Crypto::CommitTransparent(slate.fee));

		//return Crypto::AddCommitments(outputCommitments, inputCommitments);
	}

	static Commitment CalculateExcess(const std::vector<SlateSignature>& signatures)
	{
		std::vector<PublicKey> pubkeys;
		std::transform(
			signatures.cbegin(), signatures.cend(),
			std::back_inserter(pubkeys),
			[](const SlateSignature& signature) {
				WALLET_INFO_F("Adding Pubkey: {}", signature.excess);
				return signature.excess;
			}
		);

		WALLET_INFO_F("Sum Pubkeys: {}", Crypto::AddPublicKeys(pubkeys));
		return Crypto::ToCommitment(Crypto::AddPublicKeys(pubkeys));
	}
};