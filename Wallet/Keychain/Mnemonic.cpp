#include "Mnemonic.h"
#include "WordList.h"

#include <Crypto/Crypto.h>

// TODO: Apply password
SecureString Mnemonic::CreateMnemonic(const std::vector<unsigned char>& entropy, const std::optional<SecureString>& password)
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