#include "KeyChain.h"
#include "SeedEncrypter.h"
#include "KeyGenerator.h"

#include <Common/Util/VectorUtil.h>

KeyChain::KeyChain(const Config& config, PrivateExtKey&& masterKey, SecretKey&& bulletProofNonce)
	: m_config(config), m_masterKey(std::move(masterKey)), m_bulletProofNonce(std::move(bulletProofNonce))
{

}

KeyChain KeyChain::FromSeed(const Config& config, const SecretKey& masterSeed)
{
	PrivateExtKey masterKey = KeyGenerator(config).GenerateMasterKey(masterSeed);
	SecretKey bulletProofNonce = *Crypto::BlindSwitch(masterKey.GetPrivateKey(), 0);
	return KeyChain(config, std::move(masterKey), std::move(bulletProofNonce));
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

std::unique_ptr<RewoundProof> KeyChain::RewindRangeProof(const Commitment& commitment, const RangeProof& rangeProof) const
{
	const SecretKey nonce = CreateNonce(commitment);
	return Crypto::RewindRangeProof(commitment, rangeProof, nonce);
}

std::unique_ptr<RangeProof> KeyChain::GenerateRangeProof(const KeyChainPath& keyChainPath, const uint64_t amount, const Commitment& commitment, const SecretKey& blindingFactor) const
{
	const SecretKey nonce = CreateNonce(commitment);
	const ProofMessage proofMessage = ProofMessage::FromKeyIndices(keyChainPath.GetKeyIndices());

	return Crypto::GenerateRangeProof(amount, blindingFactor, nonce, proofMessage);
}

SecretKey KeyChain::CreateNonce(const Commitment& commitment) const
{
	return Crypto::Blake2b(commitment.GetCommitmentBytes().GetData(), m_bulletProofNonce.GetBytes().GetData());
}