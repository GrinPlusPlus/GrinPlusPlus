#pragma once

#include <Crypto/Commitment.h>
#include <Crypto/Signature.h>
#include <stdint.h>
#include <memory>

class ParticipantData
{
private:
	// Id of participant in the transaction. (For now, 0=sender, 1=rec)
	uint64_t m_participantId;
	// Public key corresponding to private blinding factor
	CBigInteger<33> m_publicBlindExcess;
	// Public key corresponding to private nonce
	CBigInteger<33> m_publicNonce;
	// Public partial signature
	std::unique_ptr<Signature> m_pPartialSignature;
	// A message for other participants
	std::string m_message;
	// Signature, created with private key corresponding to 'public_blind_excess'
	std::unique_ptr<Signature> m_pMessageSignature;
};