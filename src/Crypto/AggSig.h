#pragma once

#include "secp256k1-zkp/include/secp256k1_aggsig.h"

#include <Crypto/Commitment.h>
#include <Crypto/SecretKey.h>
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

	SecretKey GenerateSecureNonce() const;

	std::unique_ptr<CompactSignature> SignMessage(const SecretKey& secretKey, const PublicKey& publicKey, const Hash& message);
	bool VerifyMessageSignature(const CompactSignature& signature, const PublicKey& publicKey, const Hash& message) const;

	std::unique_ptr<CompactSignature> CalculatePartialSignature(const SecretKey& secretKey, const SecretKey& secretNonce, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message);
	bool VerifyPartialSignature(const CompactSignature& partialSignature, const PublicKey& publicKey, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message) const;

	std::unique_ptr<Signature> AggregateSignatures(const std::vector<CompactSignature>& signatures, const PublicKey& sumPubNonces) const;
	bool VerifyAggregateSignatures(const std::vector<const Signature*>& signatures, const std::vector<const Commitment*>& publicKeys, const std::vector<const Hash*>& messages) const;
	bool VerifyAggregateSignature(const Signature& signature, const PublicKey& sumPubKeys, const Hash& message) const;

private:
	AggSig();
	~AggSig();

	std::vector<secp256k1_ecdsa_signature> ParseCompactSignatures(const std::vector<CompactSignature>& signatures) const;

	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
};