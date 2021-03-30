#pragma once

#include <secp256k1-zkp/secp256k1_aggsig.h>

#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/SecretKey.h>
#include <Crypto/Models/Signature.h>
#include <Crypto/Models/PublicKey.h>
#include <Crypto/Models/Hash.h>
#include <vector>
#include <memory>
#include <shared_mutex>

// Forward Declarations
typedef struct secp256k1_context_struct secp256k1_context;

class AggSig
{
public:
	static AggSig& GetInstance();
	AggSig();
	~AggSig();

	SecretKey GenerateSecureNonce() const;

	std::unique_ptr<Signature> BuildSignature(const SecretKey& secretKey, const Commitment& commitment, const Hash& message);

	CompactSignature CalculatePartialSignature(const SecretKey& secretKey, const SecretKey& secretNonce, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message);
	bool VerifyPartialSignature(const CompactSignature& partialSignature, const PublicKey& publicKey, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message) const;

	std::unique_ptr<Signature> AggregateSignatures(const std::vector<CompactSignature>& signatures, const PublicKey& sumPubNonces) const;
	bool VerifyAggregateSignatures(const std::vector<const Signature*>& signatures, const std::vector<const Commitment*>& publicKeys, const std::vector<const Hash*>& messages) const;
	bool VerifyAggregateSignature(const Signature& signature, const PublicKey& sumPubKeys, const Hash& message) const;

	std::vector<secp256k1_ecdsa_signature> ParseCompactSignatures(const std::vector<CompactSignature>& signatures) const;
	CompactSignature ToCompact(const Signature& signature) const;

private:
	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
};