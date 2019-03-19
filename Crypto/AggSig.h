#pragma once

#include "secp256k1-zkp/include/secp256k1_aggsig.h"

#include <Crypto/Commitment.h>
#include <Crypto/BlindingFactor.h>
#include <Crypto/Signature.h>
#include <Crypto/PublicKey.h>
#include <Crypto/Hash.h>
#include <vector>
#include <memory>
#include <shared_mutex>

// Forward Declarations
typedef struct secp256k1_context_struct secp256k1_context;

class AggSig
{
public:
	static AggSig& GetInstance();

	std::unique_ptr<BlindingFactor> GenerateSecureNonce() const;

	std::unique_ptr<Signature> SignSingle(const BlindingFactor& secretKey, const BlindingFactor& secretNonce, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message) const;
	bool VerifyPartialSignature(const Signature& partialSignature, const PublicKey& publicKey, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message) const;

	std::unique_ptr<Signature> AggregateSignatures(const std::vector<Signature>& signatures, const PublicKey& sumPubNonces) const;
	bool VerifyAggregateSignature(const Signature& signature, const Commitment& commitment, const Hash& message) const;
	bool VerifyAggregateSignature(const Signature& signature, const PublicKey& sumPubKeys, const Hash& message) const;

private:
	AggSig();
	~AggSig();

	std::vector<secp256k1_ecdsa_signature> ParseSignatures(const std::vector<Signature>& signatures) const;
	std::unique_ptr<Signature> SerializeSignature(const secp256k1_ecdsa_signature& signature) const;

	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
};