#pragma once

#include <Wallet/ParticipantData.h>
#include <Crypto/Crypto.h>
#include <Crypto/CryptoUtil.h>

class SignatureUtil
{
public:
	static std::unique_ptr<Signature> GeneratePartialSignature(const SecretKey& secretKey, const SecretKey& secretNonce, const std::vector<ParticipantData>& participants, const Hash& message)
	{
		std::vector<PublicKey> pubKeys;
		std::vector<PublicKey> pubNonces;

		for (const ParticipantData& participantData : participants)
		{
			pubKeys.push_back(participantData.GetPublicBlindExcess());
			pubNonces.push_back(participantData.GetPublicNonce());
		}

		const PublicKey sumPubKeys = CryptoUtil::AddPublicKeys(pubKeys);
		const PublicKey sumPubNonces = CryptoUtil::AddPublicKeys(pubNonces);

		return Crypto::CalculatePartialSignature(secretKey, secretNonce, sumPubKeys, sumPubNonces, message);
	}

	static std::unique_ptr<Signature> AggregateSignatures(const std::vector<ParticipantData>& participants)
	{
		std::vector<Signature> signatures;
		std::vector<PublicKey> pubNonces;

		for (const ParticipantData& participantData : participants)
		{
			pubNonces.push_back(participantData.GetPublicNonce());

			if (participantData.GetPartialSignature().has_value())
			{
				signatures.push_back(participantData.GetPartialSignature().value());
			}
		}

		const PublicKey sumPubNonces = CryptoUtil::AddPublicKeys(pubNonces);

		return Crypto::AggregateSignatures(signatures, sumPubNonces);
	}

	static bool VerifyPartialSignatures(const std::vector<ParticipantData>& participants, const Hash& message)
	{
		std::vector<PublicKey> pubKeys;
		std::vector<PublicKey> pubNonces;

		for (const ParticipantData& participantData : participants)
		{
			pubKeys.push_back(participantData.GetPublicBlindExcess());
			pubNonces.push_back(participantData.GetPublicNonce());
		}

		const PublicKey sumPubKeys = CryptoUtil::AddPublicKeys(pubKeys);
		const PublicKey sumPubNonces = CryptoUtil::AddPublicKeys(pubNonces);

		for (const ParticipantData& participant : participants)
		{
			if (participant.GetPartialSignature().has_value())
			{
				if (!Crypto::VerifyPartialSignature(participant.GetPartialSignature().value(), participant.GetPublicBlindExcess(), sumPubKeys, sumPubNonces, message))
				{
					return false;
				}
			}
		}

		return true;
	}

	static bool VerifyMessageSignatures(const std::vector<ParticipantData>& participants)
	{
		for (const ParticipantData& participant : participants)
		{
			if (participant.GetMessageText().has_value())
			{
				const Hash message = Crypto::Blake2b(std::vector<unsigned char>(participant.GetMessageText().value().cbegin(), participant.GetMessageText().value().cend()));
				if (!Crypto::VerifyMessageSignature(participant.GetMessageSignature().value(), participant.GetPublicBlindExcess(), message))
				{
					return false;
				}
			}
		}

		return true;
	}

	static bool VerifyAggregateSignature(const Signature& aggregateSignature, const std::vector<ParticipantData>& participants, const Hash& message)
	{
		std::vector<PublicKey> pubKeys;

		for (const ParticipantData& participantData : participants)
		{
			pubKeys.push_back(participantData.GetPublicBlindExcess());
		}

		const PublicKey sumPubKeys = CryptoUtil::AddPublicKeys(pubKeys);

		return Crypto::VerifyAggregateSignature(aggregateSignature, sumPubKeys, message);
	}
};