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
	KeyChain(const Config& config, EncryptedSeed&& encryptedSeed);
	KeyChain(const Config& config, const EncryptedSeed& encryptedSeed);

	std::unique_ptr<PrivateExtKey> DerivePrivateKey(const SecureString& password, const KeyChainPath& keyPath) const;
	std::unique_ptr<PublicExtKey> DerivePublicKey(const PublicExtKey& publicKey, const KeyChainPath& keyPath) const;

	std::unique_ptr<RangeProof> GenerateRangeProof(const KeyChainPath& keyChainPath, const SecureString& password, const uint64_t amount, const Commitment& commitment, const BlindingFactor& blindingFactor) const;

private:
	CBigInteger<32> CreateNonce(const Commitment& commitment, const SecureString& password) const;

	const Config& m_config;
	EncryptedSeed m_encryptedSeed;
};