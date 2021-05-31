#include <Wallet/Keychain/KeyChain.h>
#include "SeedEncrypter.h"
#include "KeyGenerator.h"

#include <Core/Global.h>
#include <Crypto/CSPRNG.h>
#include <Crypto/ED25519.h>
#include <Crypto/Crypto.h>
#include <Crypto/Hasher.h>
#include <Wallet/Exceptions/KeyChainException.h>
#include <Core/Exceptions/UnimplementedException.h>

KeyChain::KeyChain(PrivateExtKey&& masterKey, SecretKey&& bulletProofNonce)
	: m_masterKey(std::move(masterKey)), m_bulletProofNonce(std::move(bulletProofNonce))
{

}

KeyChain KeyChain::FromSeed(const SecureVector& masterSeed)
{
	PrivateExtKey masterKey = KeyGenerator(Global::GetConfig()).GenerateMasterKey(masterSeed);
	SecretKey bulletProofNonce = Crypto::BlindSwitch(masterKey.GetPrivateKey(), 0);
	return KeyChain(std::move(masterKey), std::move(bulletProofNonce));
}

KeyChain KeyChain::FromRandom()
{
	SecretKey masterSeed(CSPRNG::GenerateRandom32());
	return KeyChain::FromSeed(masterSeed.GetSecure());
}

SecretKey KeyChain::DerivePrivateKey(const KeyChainPath& keyPath) const
{
	KeyGenerator keygen(Global::GetConfig());

	PrivateExtKey privateKey(m_masterKey);
	for (const uint32_t childIndex : keyPath.GetKeyIndices())
	{
		privateKey = keygen.GenerateChildPrivateKey(privateKey, childIndex);
	}

	return privateKey.GetPrivateKey();
}

SecretKey KeyChain::DerivePrivateKey(const KeyChainPath& keyPath, const uint64_t amount) const
{
	return Crypto::BlindSwitch(DerivePrivateKey(keyPath), amount);
}

ed25519_keypair_t KeyChain::DeriveED25519Key(const KeyChainPath& keyPath) const
{
	SecretKey preSeed = DerivePrivateKey(keyPath);
	SecretKey seed = Hasher::Blake2b(preSeed.GetVec());

	return ED25519::CalculateKeypair(seed);
}

std::unique_ptr<RewoundProof> KeyChain::RewindRangeProof(const Commitment& commitment, const RangeProof& rangeProof, const EBulletproofType& bulletproofType) const
{
	if (bulletproofType == EBulletproofType::ORIGINAL)
	{
		const SecretKey nonce = CreateNonce(commitment, m_bulletProofNonce);
		return Crypto::RewindRangeProof(commitment, rangeProof, nonce);
	}
	else if (bulletproofType == EBulletproofType::ENHANCED)
	{
		PublicKey masterPublicKey = Crypto::CalculatePublicKey(m_masterKey.GetPrivateKey());

		const SecretKey rewindNonceHash = Hasher::Blake2b(masterPublicKey.GetCompressedVec());

		return Crypto::RewindRangeProof(commitment, rangeProof, CreateNonce(commitment, rewindNonceHash));
	}

	throw UNIMPLEMENTED_EXCEPTION;
}

RangeProof KeyChain::GenerateRangeProof(
	const KeyChainPath& keyChainPath, 
	const uint64_t amount, 
	const Commitment& commitment, 
	const SecretKey& blindingFactor, 
	const EBulletproofType& bulletproofType) const
{
	const ProofMessage proofMessage = ProofMessage::FromKeyIndices(keyChainPath.GetKeyIndices(), bulletproofType);

	if (bulletproofType == EBulletproofType::ORIGINAL)
	{
		const SecretKey nonce = CreateNonce(commitment, m_bulletProofNonce);

		return Crypto::GenerateRangeProof(amount, blindingFactor, nonce, nonce, proofMessage);
	}
	else if (bulletproofType == EBulletproofType::ENHANCED)
	{
		const SecretKey privateNonceHash = Hasher::Blake2b(m_masterKey.GetPrivateKey().GetVec());

		PublicKey masterPublicKey = Crypto::CalculatePublicKey(m_masterKey.GetPrivateKey());
		const SecretKey rewindNonceHash = Hasher::Blake2b(masterPublicKey.GetCompressedVec());

		return Crypto::GenerateRangeProof(amount, blindingFactor, CreateNonce(commitment, privateNonceHash), CreateNonce(commitment, rewindNonceHash), proofMessage);
	}
	
	throw UNIMPLEMENTED_EXCEPTION;
}

SecretKey KeyChain::CreateNonce(const Commitment& commitment, const SecretKey& nonceHash) const
{
	return Hasher::Blake2b(commitment.GetVec(), nonceHash.GetVec());
}