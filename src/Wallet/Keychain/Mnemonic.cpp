#include "Mnemonic.h"
#include "WordList.h"

#include <Wallet/Exceptions/KeyChainException.h>
#include <Common/GrinStr.h>
#include <Crypto/Hasher.h>

SecureString Mnemonic::CreateMnemonic(const uint8_t* entropy, const size_t entropy_len)
{
	if (entropy_len % 4 != 0)
	{
		throw KEYCHAIN_EXCEPTION("Entropy was of incorrect length.");
	}

	const size_t entropyBits = (entropy_len * 8);
	const size_t checksumBits = (entropyBits / 32);
	const size_t numWords = ((entropyBits + checksumBits) / 11);

	std::vector<uint16_t, secure_allocator<uint16_t>> wordIndices(numWords);

	size_t loc = 0;
	for (size_t i = 0; i < entropy_len; i++)
	{
		const uint8_t byte = entropy[i];
		for (uint8_t j = 8; j > 0; j--)
		{
			uint16_t bit = (byte & (1 << (j - 1))) == 0 ? 0 : 1;
			wordIndices[loc / 11] |= bit << (10 - (loc % 11));
			loc += 1;
		}
	}

	const uint8_t mask = (uint8_t)((1 << checksumBits) - 1);
	const uint32_t checksum = (Hasher::SHA256(entropy, entropy_len)[0] >> (8 - checksumBits)) & mask;
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

SecureVector Mnemonic::ToEntropy(const SecureString& walletWords)
{
	std::vector<GrinStr> words = GrinStr(walletWords.c_str()).Trim().Split(" ");
	const size_t numWords = words.size();
	if (numWords < 12 || numWords > 24 || numWords % 3 != 0)
	{
		throw KEYCHAIN_EXCEPTION("Invalid number of words provided.");
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
			throw KEYCHAIN_EXCEPTION("Word not found.");
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

	const uint32_t actualChecksum = (Hasher::SHA256((const std::vector<uint8_t>&)entropy)[0] >> (8 - checksumBits)) & mask;
	if (actualChecksum != expectedChecksum)
	{
		throw KEYCHAIN_EXCEPTION("Invalid checksum.");
	}

	return entropy;
}