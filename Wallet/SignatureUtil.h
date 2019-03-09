#pragma once

#include <Wallet/ParticipantData.h>
#include <Crypto/Crypto.h>
#include <Crypto/CryptoUtil.h>

class SignatureUtil
{
public:
	static std::unique_ptr<Signature> GeneratePartialSignature(const BlindingFactor& secretKey, const BlindingFactor& secretNonce, const std::vector<ParticipantData>& participants, const Hash& message)
	{
		std::vector<CBigInteger<33>> pubKeys;
		std::vector<CBigInteger<33>> pubNonces;

		for (const ParticipantData& participantData : participants)
		{
			pubKeys.push_back(participantData.GetPublicBlindExcess());
			pubNonces.push_back(participantData.GetPublicNonce());
		}

		const CBigInteger<33> sumPubKeys = CryptoUtil::AddPublicKeys(pubKeys);
		const CBigInteger<33> sumPubNonces = CryptoUtil::AddPublicKeys(pubNonces);

		return Crypto::CalculatePartialSignature(secretKey, secretNonce, sumPubKeys, sumPubNonces, message);
	}

	static std::unique_ptr<Signature> AggregateSignatures(const std::vector<ParticipantData>& participants)
	{
		std::vector<Signature> signatures;
		std::vector<CBigInteger<33>> pubNonces;

		for (const ParticipantData& participantData : participants)
		{
			pubNonces.push_back(participantData.GetPublicNonce());

			if (participantData.GetPartialSignature().has_value())
			{
				signatures.push_back(participantData.GetPartialSignature().value());
			}
		}

		const CBigInteger<33> sumPubNonces = CryptoUtil::AddPublicKeys(pubNonces);

		return Crypto::AggregateSignatures(signatures, sumPubNonces);
	}

	static bool VerifyPartialSignatures(const std::vector<ParticipantData>& participants)
	{
		for (const ParticipantData& participant : participants)
		{
			if (participant.GetPartialSignature().has_value())
			{
				// TODO: Verify sig
			}
		}

		return true;
	}
};