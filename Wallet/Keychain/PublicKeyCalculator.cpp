#include "PublicKeyCalculator.h"

#include <Crypto/Crypto.h>

std::unique_ptr<PublicExtKey> PublicKeyCalculator::DeterminePublicKey(const PrivateExtKey& privateKey) const
{
	std::unique_ptr<CBigInteger<33>> pCompressedPublicKey = Crypto::SECP256K1_CalculateCompressedPublicKey(privateKey.GetPrivateKey());
	if (pCompressedPublicKey != nullptr)
	{
		CBigInteger<32> chainCode = privateKey.GetChainCode();
		CBigInteger<33> compressedBytes = *pCompressedPublicKey;
		return std::make_unique<PublicExtKey>(privateKey.GetNetwork(), privateKey.GetDepth(), privateKey.GetParentFingerprint(), privateKey.GetChildNumber(), std::move(chainCode), std::move(compressedBytes));
	}

	return std::unique_ptr<PublicExtKey>(nullptr);
}