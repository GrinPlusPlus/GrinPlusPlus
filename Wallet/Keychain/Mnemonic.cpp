#include "Mnemonic.h"
#include "WordList.h"

#include <Crypto/Crypto.h>
#include <Common/Util/VectorUtil.h>

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

	const std::vector<unsigned char> entropyAndCS = VectorUtil::Concat(entropy, Crypto::SHA256(entropy).GetData());

	std::vector<uint16_t> wordIndices(numWords);
	size_t loc = 0;

	for (const uint8_t byte : entropyAndCS)
	{
		for (uint8_t i = 7; i >= 0; i--)
		{
			uint16_t bit = (byte & (1 << i)) == 0 ? 0 : 1;
			wordIndices[loc / 11] |= bit << (10 - (loc % 11));
			loc += 1;
		}
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