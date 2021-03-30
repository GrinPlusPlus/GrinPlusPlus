#pragma once

#include <secp256k1-zkp/secp256k1_commitment.h>

#include <Crypto/Models/BlindingFactor.h>
#include <Crypto/Models/SecretKey.h>
#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/PublicKey.h>
#include <shared_mutex>

// Forward Declarations
typedef struct secp256k1_context_struct secp256k1_context;

class Pedersen
{
public:
	static Pedersen& GetInstance();
	Pedersen();
	~Pedersen();

	Commitment PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const;
	Commitment PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const;
	BlindingFactor PedersenBlindSum(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative) const;

	SecretKey BlindSwitch(const SecretKey& secretKey, const uint64_t amount) const;

	Commitment ToCommitment(const PublicKey& publicKey) const;

	static std::vector<secp256k1_pedersen_commitment*> ConvertCommitments(const secp256k1_context& context, const std::vector<Commitment>& commitments);
	static void CleanupCommitments(std::vector<secp256k1_pedersen_commitment*>& commitments);

private:
	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
};