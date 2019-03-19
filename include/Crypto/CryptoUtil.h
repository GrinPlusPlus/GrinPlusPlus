#pragma once

#include <Crypto/Crypto.h>
#include <Crypto/CryptoException.h>

class CryptoUtil
{
public:
	static BlindingFactor AddBlindingFactors(const BlindingFactor* pPositive, const BlindingFactor* pNegative)
	{
		std::vector<BlindingFactor> positive;
		if (pPositive != nullptr)
		{
			positive.push_back(*pPositive);
		}

		std::vector<BlindingFactor> negative;
		if (pNegative != nullptr)
		{
			negative.push_back(*pNegative);
		}

		std::unique_ptr<BlindingFactor> pBlindingFactor = Crypto::AddBlindingFactors(positive, negative);
		if (pBlindingFactor == nullptr)
		{
			throw CryptoException();
		}

		return *pBlindingFactor;
	}

	static PublicKey AddPublicKeys(const std::vector<PublicKey>& publicKeys)
	{
		std::unique_ptr<PublicKey> pPublicKey = Crypto::AddPublicKeys(publicKeys);
		if (pPublicKey == nullptr)
		{
			throw CryptoException();
		}

		return *pPublicKey;
	}
};