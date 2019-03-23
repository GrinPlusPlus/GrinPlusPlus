#pragma once

#include <Crypto/SecretKey.h>
#include <Crypto/ProofMessage.h>
#include <stdint.h>

class RewoundProof
{
public:
	RewoundProof(const uint64_t amount, SecretKey&& blindingFactor, ProofMessage&& proofMessage)
		: m_amount(amount), m_blindingFactor(std::move(blindingFactor)), m_proofMessage(std::move(proofMessage))
	{

	}

	inline uint64_t GetAmount() const { return m_amount; }
	inline const SecretKey& GetBlindingFactor() const { return m_blindingFactor; }
	inline const ProofMessage& GetProofMessage() const { return m_proofMessage; }

private:
	uint64_t m_amount;
	SecretKey m_blindingFactor;
	ProofMessage m_proofMessage;
};