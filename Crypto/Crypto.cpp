#include <Crypto.h>

#include "Blake2.h"
#include "AES256.h"
#include "scrypt/crypto_scrypt.h"
#include "Secp256k1Wrapper.h"
#include "siphash.h"

CBigInteger<32> Crypto::Blake2b(const std::vector<unsigned char>& input)
{
	std::vector<unsigned char> tmp(32, 0);

	blake2b(&tmp[0], 32, &input[0], input.size(), nullptr, 0);

	return CBigInteger<32>(&tmp[0]);
}

std::unique_ptr<Commitment> Crypto::CommitTransparent(const uint64_t value)
{
	const BlindingFactor blindingFactor(CBigInteger<32>::ValueOf(0));

	return Secp256k1Wrapper::GetInstance().PedersenCommit(value, blindingFactor);
}

std::unique_ptr<Commitment> Crypto::CommitBlinded(const uint64_t value, const BlindingFactor& blindingFactor)
{
	return Secp256k1Wrapper::GetInstance().PedersenCommit(value, blindingFactor);
}

std::unique_ptr<Commitment> Crypto::AddCommitments(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative)
{
	const Commitment zeroCommitment(CBigInteger<33>::ValueOf(0));

	std::vector<Commitment> sanitizedPositive;
	for (const Commitment& positiveCommitment : positive)
	{
		if (positiveCommitment != zeroCommitment)
		{
			sanitizedPositive.push_back(positiveCommitment);
		}
	}

	std::vector<Commitment> sanitizedNegative;
	for (const Commitment& negativeCommitment : negative)
	{
		if (negativeCommitment != zeroCommitment)
		{
			sanitizedNegative.push_back(negativeCommitment);
		}
	}

	return Secp256k1Wrapper::GetInstance().PedersenCommitSum(sanitizedPositive, sanitizedNegative);
}

bool Crypto::VerifyRangeProofs(const std::vector<Commitment>& commitments, const std::vector<RangeProof>& rangeProofs)
{
	// TODO: Implement
	return true;
}

bool Crypto::VerifyKernelSignature(const Signature& signature, const Commitment& publicKey, const Hash& message)
{
	// TODO: Implement
	return true;
}

uint64_t Crypto::SipHash24(const uint64_t k0, const uint64_t k1, const std::vector<unsigned char>& data)
{
	const std::vector<uint64_t>& key = { k0, k1 };

	std::vector<unsigned char> output(24);
	return siphash24(&key[0], &data[0], data.size());
}

std::vector<unsigned char> Crypto::AES256_Encrypt(const std::vector<unsigned char>& input, const std::vector<unsigned char>& key)
{
	std::vector<unsigned char> output;
	Aes256::encrypt(key, input, output);

	return output;
}

std::vector<unsigned char> Crypto::AES256_Decrypt(const std::vector<unsigned char>& encrypted, const std::vector<unsigned char>& key)
{
	std::vector<unsigned char> output;
	Aes256::decrypt(key, encrypted, output);

	return output;
}

CBigInteger<64> Crypto::Scrypt(const std::vector<unsigned char>& input, const std::vector<unsigned char>& salt)
{
	const uint32_t N = 16384;// TODO: Before releasing, use 65536 (2^16) at least. Could potentially calculate this dynamically.
	const uint32_t r = 8;
	const uint32_t p = 1;

	std::vector<unsigned char> buffer(64);
	if (crypto_scrypt(&input[0], input.size(), &salt[0], salt.size(), N, r, p, &buffer[0], buffer.size()) == 0)
	{
		return CBigInteger<64>(&buffer[0]);
	}

	return CBigInteger<64>();
}