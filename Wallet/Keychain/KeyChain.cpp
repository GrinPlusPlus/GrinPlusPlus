#include "KeyChain.h"
#include "SeedEncrypter.h"
#include "KeyGenerator.h"

#include <VectorUtil.h>

KeyChain::KeyChain(const Config& config, EncryptedSeed&& encryptedSeed)
	: m_config(config), m_encryptedSeed(encryptedSeed)
{

}

KeyChain::KeyChain(const Config& config, const EncryptedSeed& encryptedSeed)
	: m_config(config), m_encryptedSeed(encryptedSeed)
{

}

std::unique_ptr<PrivateExtKey> KeyChain::DerivePrivateKey(const SecureString& password, const KeyChainPath& keyPath) const
{
	const CBigInteger<32> masterSeed = SeedEncrypter().DecryptWalletSeed(m_encryptedSeed, password);

	PrivateExtKey masterKey = KeyGenerator(m_config).GenerateMasterKey(masterSeed);

	std::unique_ptr<PrivateExtKey> pPrivateKey = std::make_unique<PrivateExtKey>(std::move(masterKey));
	for (const uint32_t childIndex : keyPath.GetKeyIndices())
	{
		if (pPrivateKey == nullptr)
		{
			return std::unique_ptr<PrivateExtKey>(nullptr);
		}

		pPrivateKey = KeyGenerator(m_config).GenerateChildPrivateKey(*pPrivateKey, childIndex);
	}

	// TODO: Generate switch commitment
	return pPrivateKey;
}

std::unique_ptr<RangeProof> KeyChain::GenerateRangeProof(const KeyChainPath& keyChainPath, const SecureString& password, const uint64_t amount, const Commitment& commitment, const BlindingFactor& blindingFactor) const
{
	CBigInteger<32> nonce = CreateNonce(commitment, password);
	ProofMessage proofMessage = ProofMessage::FromKeyIndices(keyChainPath.GetKeyIndices());

	return Crypto::GenerateRangeProof(amount, blindingFactor, nonce, proofMessage);
}

CBigInteger<32> KeyChain::CreateNonce(const Commitment& commitment, const SecureString& password) const
{
	std::unique_ptr<PrivateExtKey> pRootKey = DerivePrivateKey(password, KeyChainPath::FromString("m"));
	std::vector<unsigned char> input = VectorUtil::Concat(commitment.GetCommitmentBytes().GetData(), pRootKey->ToBlindingFactor().GetBlindingFactorBytes().GetData());

	return Crypto::Blake2b(input);
}