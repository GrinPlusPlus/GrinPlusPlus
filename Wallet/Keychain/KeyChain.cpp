#include "KeyChain.h"
#include "SeedEncrypter.h"
#include "KeyGenerator.h"

#include <VectorUtil.h>

KeyChain::KeyChain(const Config& config)
	: m_config(config)
{

}

std::unique_ptr<PrivateExtKey> KeyChain::DerivePrivateKey(const CBigInteger<32>& masterSeed, const KeyChainPath& keyPath) const
{
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

std::unique_ptr<RangeProof> KeyChain::GenerateRangeProof(const CBigInteger<32>& masterSeed, const KeyChainPath& keyChainPath, const uint64_t amount, const Commitment& commitment, const BlindingFactor& blindingFactor) const
{
	CBigInteger<32> nonce = CreateNonce(masterSeed, commitment);
	ProofMessage proofMessage = ProofMessage::FromKeyIndices(keyChainPath.GetKeyIndices());

	return Crypto::GenerateRangeProof(amount, blindingFactor, nonce, proofMessage);
}

CBigInteger<32> KeyChain::CreateNonce(const CBigInteger<32>& masterSeed, const Commitment& commitment) const
{
	std::unique_ptr<PrivateExtKey> pRootKey = DerivePrivateKey(masterSeed, KeyChainPath::FromString("m"));
	std::vector<unsigned char> input = VectorUtil::Concat(commitment.GetCommitmentBytes().GetData(), pRootKey->ToBlindingFactor().GetBlindingFactorBytes().GetData());

	return Crypto::Blake2b(input);
}