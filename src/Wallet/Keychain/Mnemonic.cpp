#include "Mnemonic.h"
#include "WordList.h"

#include <Common/Util/StringUtil.h>
#include <Crypto/Crypto.h>

SecureString Mnemonic::CreateMnemonic(const std::vector<unsigned char>& entropy)
{
	if (entropy.size() % 4 != 0)
	{
		return SecureString("");
	}

	const size_t entropyBits = (entropy.size() * 8);
	const size_t checksumBits = (entropyBits / 32);
	const size_t numWords = ((entropyBits + checksumBits) / 11);

	std::vector<uint16_t, secure_allocator<uint16_t>> wordIndices(numWords);

	size_t loc = 0;
	for (const uint8_t byte : entropy)
	{
		for (uint8_t i = 8; i > 0; i--)
		{
			uint16_t bit = (byte & (1 << (i - 1))) == 0 ? 0 : 1;
			wordIndices[loc / 11] |= bit << (10 - (loc % 11));
			loc += 1;
		}
	}

	const uint8_t mask = (uint8_t)((1 << checksumBits) - 1);
	const uint32_t checksum = (Crypto::SHA256(entropy)[0] >> (8 - checksumBits)) & mask;
	for (size_t i = checksumBits; i > 0; i--)
	{
		uint16_t bit = (checksum & (1 << (i - 1))) == 0 ? 0 : 1;
		wordIndices[loc / 11] |= bit << (10 - (loc % 11));
		loc += 1;
	}

	SecureString result("");
	for (size_t i = 0; i < numWords; i++)
	{
		if (i != 0)
		{
			result += " ";
		}

		result += WORD_LIST[wordIndices[i]];
	}

	return result;
}

std::optional<SecureVector> Mnemonic::ToEntropy(const SecureString& walletWords)
{
	const std::vector<std::string> words = StringUtil::Split(std::string(walletWords), " ");
	const size_t numWords = words.size();
	if (numWords < 12 || numWords > 24 || numWords % 3 != 0)
	{
		return std::nullopt;
	}

	// u11 vector of indexes for each word
	std::vector<uint16_t, secure_allocator<uint16_t>> wordIndices(numWords);
	for (size_t i = 0; i < numWords; i++)
	{
		bool wordFound = false;
		for (uint16_t j = 0; j < WORD_LIST.size(); j++)
		{
			if (WORD_LIST[j] == words[i])
			{
				wordIndices[i] = j;
				wordFound = true;
				break;
			}
		}

		if (!wordFound)
		{
			return std::nullopt;
		}
	}

	const size_t checksumBits = numWords / 3;
	const uint8_t mask = (uint8_t)((1 << checksumBits) - 1);
	const uint8_t expectedChecksum = ((uint8_t)wordIndices.back()) & mask;

	const size_t dataLength = (((11 * numWords) - checksumBits) / 8) - 1;
	SecureVector entropy(dataLength + 1, 0);
	entropy[dataLength] = (uint8_t)(wordIndices.back() >> checksumBits);

	size_t loc = 11 - checksumBits;

	// cast vector of u11 as u8
	for (size_t i = numWords - 1; i > 0; i--)
	{
		for (size_t k = 0; k < 11; k++)
		{
			const uint8_t bit = (wordIndices[i - 1] & (1 << k)) == 0 ? 0 : 1;
			entropy[dataLength - (loc / 8)] |= (bit << (loc % 8));
			loc += 1;
		}
	}

	const uint32_t actualChecksum = (Crypto::SHA256((const std::vector<unsigned char>&)entropy)[0] >> (8 - checksumBits)) & mask;
	if (actualChecksum != expectedChecksum)
	{
		return std::nullopt;
	}

	return std::make_optional<SecureVector>(std::move(entropy));
}