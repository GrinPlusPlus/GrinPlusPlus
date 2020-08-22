#pragma once

#include <Common/Logger.h>
#include <Core/Exceptions/WalletException.h>
#include <Crypto/Crypto.h>
#include <Wallet/Models/Slate/SlateSignature.h>

class SignatureUtil
{
public:
	static Signature AggregateSignatures(const Slate& slate)
	{
		std::vector<CompactSignature> signatures;
		std::vector<PublicKey> pubNonces;

		for (const auto& sig : slate.sigs)
		{
			if (sig.partialOpt.has_value()) {
				WALLET_INFO_F("Aggregating signature: {}", sig.partialOpt.value());
				signatures.push_back(sig.partialOpt.value());
			} else {
				WALLET_WARNING_F("No signature found for: {}", sig.excess);
			}
		}

		const PublicKey sumPubNonces = slate.CalculateTotalNonce();

		auto pAggSignature = Crypto::AggregateSignatures(signatures, sumPubNonces);
		if (pAggSignature == nullptr) {
			throw WALLET_EXCEPTION_F("Failed to aggregate signatures for {}", slate);
		}

		return *pAggSignature;
	}

	static bool VerifyPartialSignatures(const std::vector<SlateSignature>& sigs, const Hash& message)
	{
		std::vector<PublicKey> pubKeys;
		std::vector<PublicKey> pubNonces;

		for (const auto& sig : sigs)
		{
			pubKeys.push_back(sig.excess);
			pubNonces.push_back(sig.nonce);
		}

		const PublicKey sumPubKeys = Crypto::AddPublicKeys(pubKeys);
		const PublicKey sumPubNonces = Crypto::AddPublicKeys(pubNonces);

		for (const auto& sig : sigs)
		{
			if (sig.partialOpt.has_value())
			{
				if (!Crypto::VerifyPartialSignature(sig.partialOpt.value(), sig.excess, sumPubKeys, sumPubNonces, message))
				{
					WALLET_ERROR_F(
						"Partial signature {} invalid for excess {}, commitment {}, pubkey_sum {}, nonce_sum {}, and message {}",
						sig.partialOpt.value(),
						sig.excess,
						Crypto::ToCommitment(sumPubKeys),
						sumPubKeys,
						sumPubNonces,
						message.ToHex()
					);
					return false;
				}
			}
		}

		return true;
	}
};