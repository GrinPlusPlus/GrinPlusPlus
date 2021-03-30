#pragma once

#include "BulletProofsCache.h"

#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/RangeProof.h>
#include <Crypto/Models/BlindingFactor.h>
#include <Crypto/Models/ProofMessage.h>
#include <Crypto/Models/RewoundProof.h>
#include <shared_mutex>

// Forward Declarations
typedef struct secp256k1_context_struct secp256k1_context;
struct secp256k1_bulletproof_generators;

class Bulletproofs
{
public:
	static Bulletproofs& GetInstance();
	Bulletproofs();
	~Bulletproofs();

	bool VerifyBulletproofs(const std::vector<std::pair<Commitment, RangeProof>>& rangeProofs) const;

	RangeProof GenerateRangeProof(
		const uint64_t amount,
		const SecretKey& key,
		const SecretKey& privateNonce,
		const SecretKey& rewindNonce,
		const ProofMessage& proofMessage
	) const;

	std::unique_ptr<RewoundProof> RewindProof(
		const Commitment& commitment,
		const RangeProof& rangeProof,
		const SecretKey& nonce
	) const;

private:
	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
	secp256k1_bulletproof_generators* m_pGenerators;
	mutable BulletProofsCache m_cache;
};