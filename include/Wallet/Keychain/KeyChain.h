#pragma once

#include <Wallet/WalletDB/Models/EncryptedSeed.h>
#include <Wallet/PrivateExtKey.h>
#include <Wallet/PublicExtKey.h>
#include <Wallet/KeyChainPath.h>

#include <Crypto/Models/SecretKey.h>
#include <Crypto/Models/BlindingFactor.h>
#include <Crypto/Models/RangeProof.h>
#include <Crypto/Models/RewoundProof.h>
#include <Crypto/BulletproofType.h>
#include <Crypto/Models/ed25519_keypair.h>
#include <Crypto/ED25519.h>
#include <Common/Secure.h>
#include <vector>

class KeyChain
{
public:
	// FUTURE: Add FromMnemonic, ToMnemonic, and GetSeed methods
	static KeyChain FromSeed(const SecureVector& masterSeed);
	static KeyChain FromRandom();

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
	KeyChain(PrivateExtKey&& masterKey, SecretKey&& bulletProofNonce);

	SecretKey CreateNonce(const Commitment& commitment, const SecretKey& nonceHash) const;

	PrivateExtKey m_masterKey;
	SecretKey m_bulletProofNonce;
};