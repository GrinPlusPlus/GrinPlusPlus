#pragma once

#include <Wallet/Models/Slate/SlateSignature.h>
#include <Crypto/Crypto.h>

class SignatureUtil
{
public:
	static std::unique_ptr<CompactSignature> GeneratePartialSignature(
		const SecretKey& secretKey,
		const SecretKey& secretNonce,
		const std::vector<SlateSignature>& sigs,
		const Hash& message)
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

		return Crypto::CalculatePartialSignature(secretKey, secretNonce, sumPubKeys, sumPubNonces, message);
	}

	static std::unique_ptr<Signature> AggregateSignatures(const std::vector<SlateSignature>& sigs)
	{
		std::vector<CompactSignature> signatures;
		std::vector<PublicKey> pubNonces;

		for (const auto& sig : sigs)
		{
			pubNonces.push_back(sig.nonce);

			if (sig.partialOpt.has_value()) {
				WALLET_INFO_F("Aggregating signature: {}", sig.partialOpt.value().ToHex());
				signatures.push_back(sig.partialOpt.value());
			} else {
				WALLET_WARNING_F("No signature found for: {}", sig.excess.ToHex());
			}
		}

		const PublicKey sumPubNonces = Crypto::AddPublicKeys(pubNonces);

		return Crypto::AggregateSignatures(signatures, sumPubNonces);
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
						"Partial signature {} invalid for excess {}, pubkey_sum {}, nonce_sum {}, and message {}",
						sig.partialOpt.value().ToHex(),
						sig.excess.ToHex(),
						sumPubKeys.ToHex(),
						sumPubNonces.ToHex(),
						message.ToHex()
					);
					return false;
				}
			}
		}

		return true;
	}

	static bool VerifyAggregateSignature(const Signature& aggregateSignature, const std::vector<SlateSignature>& sigs, const Hash& message)
	{
		std::vector<PublicKey> pubKeys;

		for (const auto& sig : sigs)
		{
			pubKeys.push_back(sig.excess);
		}

		const PublicKey sumPubKeys = Crypto::AddPublicKeys(pubKeys);

		return Crypto::VerifyAggregateSignature(aggregateSignature, sumPubKeys, message);
	}
};