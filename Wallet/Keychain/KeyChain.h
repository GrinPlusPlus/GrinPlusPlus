#pragma once

#include <Wallet/EncryptedSeed.h>
#include <Wallet/PrivateExtKey.h>
#include <Wallet/PublicExtKey.h>
#include <Wallet/KeyChainPath.h>

#include <Config/Config.h>
#include <Common/Secure.h>
#include <vector>

class KeyChain
{
public:
	KeyChain(const Config& config);

	std::unique_ptr<PrivateExtKey> DerivePrivateKey(const CBigInteger<32>& masterSeed, const KeyChainPath& keyPath) const;
	std::unique_ptr<PublicExtKey> DerivePublicKey(const PublicExtKey& publicKey, const KeyChainPath& keyPath) const;

	std::unique_ptr<RangeProof> GenerateRangeProof(const CBigInteger<32>& masterSeed, const KeyChainPath& keyChainPath, const uint64_t amount, const Commitment& commitment, const BlindingFactor& blindingFactor) const;

private:
	CBigInteger<32> CreateNonce(const CBigInteger<32>& masterSeed, const Commitment& commitment) const;

	const Config& m_config;
};