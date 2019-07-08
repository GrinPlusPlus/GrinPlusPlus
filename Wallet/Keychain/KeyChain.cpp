#include "KeyChain.h"
#include "SeedEncrypter.h"
#include "KeyGenerator.h"

#include <Common/Exceptions/UnimplementedException.h>
#include <Common/Util/VectorUtil.h>

KeyChain::KeyChain(const Config& config, PrivateExtKey&& masterKey, SecretKey&& bulletProofNonce)
	: m_config(config), m_masterKey(std::move(masterKey)), m_bulletProofNonce(std::move(bulletProofNonce))
{

}

KeyChain KeyChain::FromSeed(const Config& config, const SecureVector& masterSeed)
{
	PrivateExtKey masterKey = KeyGenerator(config).GenerateMasterKey(masterSeed, EKeyChainType::DEFAULT);
	SecretKey bulletProofNonce = *Crypto::BlindSwitch(masterKey.GetPrivateKey(), 0);
	return KeyChain(config, std::move(masterKey), std::move(bulletProofNonce));
}

KeyChain KeyChain::ForGrinbox(const Config& config, const SecureVector& masterSeed)
{
	PrivateExtKey masterKey = KeyGenerator(config).GenerateMasterKey(masterSeed, EKeyChainType::DEFAULT);
	SecretKey rootKey = *Crypto::BlindSwitch(masterKey.GetPrivateKey(), 713);
	masterKey = KeyGenerator(config).GenerateMasterKey(SecureVector(rootKey.data(), rootKey.data() + rootKey.size()), EKeyChainType::GRINBOX);
	SecretKey bulletProofNonce = *Crypto::BlindSwitch(masterKey.GetPrivateKey(), 0);
	return KeyChain(config, std::move(masterKey), std::move(bulletProofNonce));
}

std::unique_ptr<SecretKey> KeyChain::DerivePrivateKey(const KeyChainPath& keyPath) const
{
	std::unique_ptr<PrivateExtKey> pPrivateKey = std::make_unique<PrivateExtKey>(PrivateExtKey(m_masterKey));
	for (const uint32_t childIndex : keyPath.GetKeyIndices())
	{
		if (pPrivateKey == nullptr)
		{
			return std::unique_ptr<SecretKey>(nullptr);
		}

		pPrivateKey = KeyGenerator(m_config).GenerateChildPrivateKey(*pPrivateKey, childIndex);
	}

	return std::make_unique<SecretKey>(SecretKey(pPrivateKey->GetPrivateKey()));
}

std::unique_ptr<SecretKey> KeyChain::DerivePrivateKey(const KeyChainPath& keyPath, const uint64_t amount) const
{
	std::unique_ptr<PrivateExtKey> pPrivateKey = std::make_unique<PrivateExtKey>(PrivateExtKey(m_masterKey));
	for (const uint32_t childIndex : keyPath.GetKeyIndices())
	{
		if (pPrivateKey == nullptr)
		{
			return std::unique_ptr<SecretKey>(nullptr);
		}

		pPrivateKey = KeyGenerator(m_config).GenerateChildPrivateKey(*pPrivateKey, childIndex);
	}

	// Generate switch commitment
	return Crypto::BlindSwitch(pPrivateKey->GetPrivateKey(), amount);
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
		std::unique_ptr<PublicKey> pMasterPublicKey = Crypto::CalculatePublicKey(m_masterKey.GetPrivateKey()); // TODO: Cache this
		if (pMasterPublicKey == nullptr)
		{
			return std::unique_ptr<RewoundProof>(nullptr);
		}

		const SecretKey rewindNonceHash = Crypto::Blake2b(pMasterPublicKey->GetCompressedBytes().GetData());

		return Crypto::RewindRangeProof(commitment, rangeProof, CreateNonce(commitment, rewindNonceHash));
	}

	throw UNIMPLEMENTED_EXCEPTION;
}

std::unique_ptr<RangeProof> KeyChain::GenerateRangeProof(
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
		const SecretKey privateNonceHash = Crypto::Blake2b(m_masterKey.GetPrivateKey().GetBytes().GetData());

		std::unique_ptr<PublicKey> pMasterPublicKey = Crypto::CalculatePublicKey(m_masterKey.GetPrivateKey()); // TODO: Cache this
		if (pMasterPublicKey == nullptr)
		{
			return std::unique_ptr<RangeProof>(nullptr);
		}

		const SecretKey rewindNonceHash = Crypto::Blake2b(pMasterPublicKey->GetCompressedBytes().GetData());

		return Crypto::GenerateRangeProof(amount, blindingFactor, CreateNonce(commitment, privateNonceHash), CreateNonce(commitment, rewindNonceHash), proofMessage);
	}
	
	throw UNIMPLEMENTED_EXCEPTION;
}

SecretKey KeyChain::CreateNonce(const Commitment& commitment, const SecretKey& nonceHash) const
{
	return Crypto::Blake2b(commitment.GetCommitmentBytes().GetData(), nonceHash.GetBytes().GetData());
}