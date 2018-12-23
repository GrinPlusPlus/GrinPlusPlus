#pragma once

#include "secp256k1-zkp/include/secp256k1_commitment.h"

#include <Crypto/Commitment.h>
#include <Crypto/BlindingFactor.h>
#include <vector>
#include <memory>

class Secp256k1Wrapper
{
public:
	static Secp256k1Wrapper& GetInstance();

	std::unique_ptr<CBigInteger<33>> CalculatePublicKey(const CBigInteger<32>& privateKey) const;
	std::unique_ptr<Commitment> PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const;
	std::unique_ptr<Commitment> PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const;

private:
	Secp256k1Wrapper();
	~Secp256k1Wrapper();

	std::vector<secp256k1_pedersen_commitment*> ConvertCommitments(const std::vector<Commitment>& commitments) const;
	void CleanupCommitments(std::vector<secp256k1_pedersen_commitment*>& commitments) const;

	secp256k1_context* m_pContext;
};