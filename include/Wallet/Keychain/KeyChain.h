#pragma once

#include <Wallet/WalletDB/Models/EncryptedSeed.h>
#include <Wallet/PrivateExtKey.h>
#include <Wallet/PublicExtKey.h>
#include <Wallet/KeyChainPath.h>

#include <Core/Config.h>
#include <Crypto/SecretKey.h>
#include <Crypto/BlindingFactor.h>
#include <Crypto/RangeProof.h>
#include <Crypto/RewoundProof.h>
#include <Crypto/BulletproofType.h>
#include <Crypto/Models/ed25519_keypair.h>
#include <Crypto/ED25519.h>
#include <Common/Secure.h>
#include <vector>

class KeyChain
{
public:
	// FUTURE: Add FromMnemonic, ToMnemonic, and GetSeed methods
	static KeyChain FromSeed(const Config& config, const SecureVector& masterSeed);
	static KeyChain FromRandom(const Config& config);

	SecretKey DerivePrivateKey(const KeyChainPath& keyPath, const uint64_t amount) const;
	SecretKey DerivePrivateKey(const KeyChainPath& keyPath) const;
	ed25519_keypair_t DeriveED25519Key(const KeyChainPath& keyPath) const;

	std::unique_ptr<RewoundProof> RewindRangeProof(
		const Commitment& commitment,
		const RangeProof& rangeProof,
		const EBulletproofType& bulletproofType
	) const;

	RangeProof GenerateRangeProof(
		const KeyChainPath& keyChainPath, 
		const uint64_t amount, 
		const Commitment& commitment, 
		const SecretKey& blindingFactor, 
		const EBulletproofType& bulletproofType
	) const;

private:
	KeyChain(const Config& config, PrivateExtKey&& masterKey, SecretKey&& bulletProofNonce);

	SecretKey CreateNonce(const Commitment& commitment, const SecretKey& nonceHash) const;

	const Config& m_config;
	PrivateExtKey m_masterKey;
	SecretKey m_bulletProofNonce;
};