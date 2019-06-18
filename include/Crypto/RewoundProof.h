#pragma once

#include <Crypto/SecretKey.h>
#include <Crypto/ProofMessage.h>
#include <stdint.h>

class RewoundProof
{
public:
	RewoundProof(const uint64_t amount, std::unique_ptr<SecretKey>&& pBlindingFactor, ProofMessage&& proofMessage)
		: m_amount(amount), m_pBlindingFactor(std::move(pBlindingFactor)), m_proofMessage(std::move(proofMessage))
	{

	}

	inline uint64_t GetAmount() const { return m_amount; }
	inline const std::unique_ptr<SecretKey>& GetBlindingFactor() const { return m_pBlindingFactor; }
	inline const ProofMessage& GetProofMessage() const { return m_proofMessage; }

private:
	uint64_t m_amount;
	std::unique_ptr<SecretKey> m_pBlindingFactor;
	ProofMessage m_proofMessage;
};