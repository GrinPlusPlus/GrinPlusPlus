#pragma once

#include "secp256k1-zkp/include/secp256k1_commitment.h"

#include <Crypto/BlindingFactor.h>
#include <Crypto/SecretKey.h>
#include <Crypto/Commitment.h>
#include <shared_mutex>

// Forward Declarations
typedef struct secp256k1_context_struct secp256k1_context;

class Pedersen
{
public:
	static Pedersen& GetInstance();

	std::unique_ptr<Commitment> PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const;
	std::unique_ptr<Commitment> PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const;
	std::unique_ptr<BlindingFactor> PedersenBlindSum(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative) const;

	std::unique_ptr<SecretKey> BlindSwitch(const SecretKey& secretKey, const uint64_t amount) const;

	static std::vector<secp256k1_pedersen_commitment*> ConvertCommitments(const secp256k1_context& context, const std::vector<Commitment>& commitments);
	static void CleanupCommitments(std::vector<secp256k1_pedersen_commitment*>& commitments);

private:
	Pedersen();
	~Pedersen();

	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
};