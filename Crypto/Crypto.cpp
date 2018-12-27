#include <Crypto.h>

#include "Blake2.h"
#include "sha256.h"
#include "ripemd160.h"
#include "hmac_sha256.h"
#include "hmac_sha512.h"
#include "aes.h"
#include "scrypt/crypto_scrypt.h"
#include "Secp256k1Wrapper.h"
#include "siphash.h"

#ifdef _WIN32
#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")
#endif

CBigInteger<32> Crypto::Blake2b(const std::vector<unsigned char>& input)
{
	std::vector<unsigned char> tmp(32, 0);

	blake2b(&tmp[0], 32, &input[0], input.size(), nullptr, 0);

	return CBigInteger<32>(&tmp[0]);
}

CBigInteger<32> Crypto::SHA256(const std::vector<unsigned char>& input)
{
	std::vector<unsigned char> sha256(32, 0);

	CSHA256().Write(input.data(), input.size()).Finalize(sha256.data());

	return CBigInteger<32>(sha256);
}

CBigInteger<20> Crypto::RipeMD160(const std::vector<unsigned char>& input)
{
	std::vector<unsigned char> ripemd(32, 0);

	CRIPEMD160().Write(input.data(), input.size()).Finalize(ripemd.data());

	return CBigInteger<20>(ripemd);
}

CBigInteger<32> Crypto::HMAC_SHA256(const std::vector<unsigned char>& key, const std::vector<unsigned char>& data)
{
	std::vector<unsigned char> result(32);

	CHMAC_SHA256(key.data(), key.size()).Write(data.data(), data.size()).Finalize(result.data());

	return CBigInteger<32>(result.data());
}

CBigInteger<64> Crypto::HMAC_SHA512(const std::vector<unsigned char>& key, const std::vector<unsigned char>& data)
{
	std::vector<unsigned char> result(64);

	CHMAC_SHA512(key.data(), key.size()).Write(data.data(), data.size()).Finalize(result.data());

	return CBigInteger<64>(std::move(result));
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
	// TODO: Implement (See: verify_bullet_proof_multi)
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

std::vector<unsigned char> Crypto::AES256_Encrypt(const std::vector<unsigned char>& input, const CBigInteger<32>& key, const CBigInteger<16>& iv)
{   
	std::vector<unsigned char> ciphertext;

	// max ciphertext len for a n bytes of plaintext is n + AES_BLOCKSIZE bytes
	ciphertext.resize(input.size() + AES_BLOCKSIZE);

	AES256CBCEncrypt enc(key.GetData().data(), iv.GetData().data(), true);
	const size_t nLen = enc.Encrypt(&input[0], input.size(), ciphertext.data());
	if (nLen < input.size())
	{
		throw std::out_of_range("Encrypted text is smaller than plaintext.");
	}

	ciphertext.resize(nLen);

	return ciphertext ;
}

std::vector<unsigned char> Crypto::AES256_Decrypt(const std::vector<unsigned char>& ciphertext, const CBigInteger<32>& key, const CBigInteger<16>& iv)
{    
	std::vector<unsigned char> plaintext;

	// plaintext will always be equal to or lesser than length of ciphertext
	size_t nLen = ciphertext.size();

	plaintext.resize(nLen);

	AES256CBCDecrypt dec(key.GetData().data(), iv.GetData().data(), true);
	nLen = dec.Decrypt(ciphertext.data(), ciphertext.size(), &plaintext[0]);
	if (nLen == 0)
	{
		throw std::runtime_error("Failed to decrypt data.");
	}

	plaintext.resize(nLen);

	return plaintext;
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

std::unique_ptr<CBigInteger<33>> Crypto::SECP256K1_CalculateCompressedPublicKey(const CBigInteger<32>& privateKey)
{
	return Secp256k1Wrapper::GetInstance().CalculatePublicKey(privateKey);
}
