#include "PublicKeyCalculator.h"

#include <Crypto/Crypto.h>

std::unique_ptr<PublicExtKey> PublicKeyCalculator::DeterminePublicKey(const PrivateExtKey& privateKey) const
{
	std::unique_ptr<PublicKey> pCompressedPublicKey = Crypto::CalculatePublicKey(privateKey.GetPrivateKey());
	if (pCompressedPublicKey != nullptr)
	{
		SecretKey chainCode = privateKey.GetChainCode();
		PublicKey compressedBytes = *pCompressedPublicKey;
		return std::make_unique<PublicExtKey>(privateKey.GetNetwork(), privateKey.GetDepth(), privateKey.GetParentFingerprint(), privateKey.GetChildNumber(), std::move(chainCode), std::move(compressedBytes));
	}

	return std::unique_ptr<PublicExtKey>(nullptr);
}