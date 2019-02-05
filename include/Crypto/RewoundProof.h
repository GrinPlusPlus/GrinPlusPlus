#pragma once

#include <Crypto/BlindingFactor.h>
#include <Crypto/ProofMessage.h>
#include <stdint.h>

class RewoundProof
{
public:
	RewoundProof(const uint64_t amount, BlindingFactor&& blindingFactor, ProofMessage&& proofMessage)
		: m_amount(amount), m_blindingFactor(std::move(blindingFactor)), m_proofMessage(std::move(proofMessage))
	{

	}

	inline uint64_t GetAmount() const { return m_amount; }
	inline const BlindingFactor& GetBlindingFactor() const { return m_blindingFactor; }
	inline const ProofMessage& GetMessage() const { return m_proofMessage; }

private:
	uint64_t m_amount;
	BlindingFactor m_blindingFactor;
	ProofMessage m_proofMessage;
};