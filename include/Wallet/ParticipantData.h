#pragma once

#include <Crypto/Commitment.h>
#include <Crypto/Signature.h>
#include <stdint.h>
#include <optional>

class ParticipantData
{
public:
	ParticipantData(const uint64_t participantId, const CBigInteger<33>& publicBlindExcess, const CBigInteger<33>& publicNonce)
		: m_participantId(participantId), m_publicBlindExcess(publicBlindExcess), m_publicNonce(publicNonce)
	{

	}

	inline uint64_t GetParticipantId() const { return m_participantId; }
	inline const CBigInteger<33>& GetPublicBlindExcess() const { return m_publicBlindExcess; }
	inline const CBigInteger<33>& GetPublicNonce() const { return m_publicNonce; }
	inline const std::optional<Signature>& GetPartialSignature() const { return m_partialSignatureOpt; }
	inline const std::optional<std::string>& GetMessageText() const { return m_messageOpt; }
	inline const std::optional<Signature>& GetMessageSignature() const { return m_messageSignatureOpt; }

	inline void AddPartialSignature(const Signature& signature) { m_partialSignatureOpt = std::make_optional<Signature>(signature); }
	inline void AddMessage(const std::string& message, const Signature& messageSignature)
	{
		m_messageOpt = std::make_optional<std::string>(message);
		m_messageSignatureOpt = std::make_optional<Signature>(messageSignature);
	}

private:
	// Id of participant in the transaction. (For now, 0=sender, 1=rec)
	uint64_t m_participantId;
	// Public key corresponding to private blinding factor
	CBigInteger<33> m_publicBlindExcess;
	// Public key corresponding to private nonce
	CBigInteger<33> m_publicNonce;
	// Public partial signature
	std::optional<Signature> m_partialSignatureOpt;
	// A message for other participants
	std::optional<std::string> m_messageOpt;
	// Signature, created with private key corresponding to 'public_blind_excess'
	std::optional<Signature> m_messageSignatureOpt;
};