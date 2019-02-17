#pragma once

#include "secp256k1-zkp/include/secp256k1_commitment.h"

#include <Crypto/Commitment.h>
#include <Crypto/BlindingFactor.h>
#include <Crypto/Signature.h>
#include <Crypto/RangeProof.h>
#include <Crypto/ProofMessage.h>
#include <Crypto/RewoundProof.h>
#include <Crypto/Hash.h>
#include <vector>
#include <memory>

// Forward Declarations
struct secp256k1_bulletproof_generators;

class Secp256k1Wrapper
{
public:
	static Secp256k1Wrapper& GetInstance();

	bool VerifySingleAggSig(const Signature& signature, const Commitment& publicKey, const Hash& message) const;
	bool VerifyBulletproofs(const std::vector<Commitment>& commitments, const std::vector<RangeProof>& rangeProofs) const;
	std::unique_ptr<CBigInteger<33>> CalculatePublicKey(const CBigInteger<32>& privateKey) const;
	std::unique_ptr<Commitment> PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const;
	std::unique_ptr<Commitment> PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const;
	std::unique_ptr<BlindingFactor> PedersenBlindSum(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative) const;
	std::unique_ptr<RangeProof> GenerateRangeProof(const uint64_t amount, const BlindingFactor& key, const CBigInteger<32>& nonce, const ProofMessage& proofMessage) const;
	std::unique_ptr<RewoundProof> RewindProof(const Commitment& commitment, const RangeProof& rangeProof, const CBigInteger<32>& nonce) const;

private:
	Secp256k1Wrapper();
	~Secp256k1Wrapper();

	std::vector<secp256k1_pedersen_commitment*> ConvertCommitments(const std::vector<Commitment>& commitments) const;
	void CleanupCommitments(std::vector<secp256k1_pedersen_commitment*>& commitments) const;

	secp256k1_context* m_pContext;
	secp256k1_bulletproof_generators* m_pGenerators;
};