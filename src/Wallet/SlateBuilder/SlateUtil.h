#pragma once

#include "SigningKeys.h"

#include <Common/Util/FunctionalUtil.h>
#include <Crypto/Models/Commitment.h>
#include <Crypto/Crypto.h>
#include <Wallet/Models/Slate/Slate.h>

class SlateUtil
{
public:
	static SigningKeys CalculateSigningKeys(
		const std::vector<OutputDataEntity>& inputs,
		const std::vector<OutputDataEntity>& outputs,
		const BlindingFactor& tx_offset)
	{
		// Calculate sum inputs blinding factors xI.
		std::vector<BlindingFactor> input_blinding_factors;
		std::transform(
			inputs.cbegin(), inputs.cend(),
			std::back_inserter(input_blinding_factors),
			[](const OutputDataEntity& input) -> BlindingFactor { return input.GetBlindingFactor(); }
		);
		BlindingFactor inputBFSum = Crypto::AddBlindingFactors(input_blinding_factors, {});

		// Calculate sum change outputs blinding factors xC.
		std::vector<BlindingFactor> output_blinding_factors;
		std::transform(
			outputs.cbegin(), outputs.cend(),
			std::back_inserter(output_blinding_factors),
			[](const OutputDataEntity& output) -> BlindingFactor { return output.GetBlindingFactor(); }
		);
		BlindingFactor outputBFSum = Crypto::AddBlindingFactors(output_blinding_factors, {});

		// Calculate total blinding excess sum for all inputs and outputs xS1 = xC - xI
		BlindingFactor total_blind_excess = Crypto::AddBlindingFactors({ outputBFSum }, { inputBFSum });

		// Subtract random kernel offset oS from xS1. Calculate xS = xS1 - oS
		SecretKey secret_key = Crypto::AddBlindingFactors({ total_blind_excess }, { tx_offset }).ToSecretKey();
		PublicKey public_key = Crypto::CalculatePublicKey(secret_key);

		// Select a random nonce kS
		SecretKey secret_nonce = Crypto::GenerateSecureNonce();
		PublicKey public_nonce = Crypto::CalculatePublicKey(secret_nonce);

		return SigningKeys{ secret_key, public_key, secret_nonce, public_nonce };
	}
};