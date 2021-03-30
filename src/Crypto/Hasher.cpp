#include <Crypto/Hasher.h>
#include <Core/Exceptions/CryptoException.h>

#include <sodium/crypto_generichash_blake2b.h>
#include <bitcoin/sha256.h>
#include <bitcoin/ripemd160.h>
#include <bitcoin/hmac_sha256.h>
#include <bitcoin/hmac_sha512.h>

#include "ThirdParty/siphash.h"

Hash Hasher::Blake2b(const std::vector<uint8_t>& input)
{
    return Blake2b(input.data(), input.size());
}

Hash Hasher::Blake2b(const uint8_t* input, const size_t len)
{
	Hash output;

	int result = crypto_generichash_blake2b(output.data(), output.size(), input, len, nullptr, 0);
	if (result != 0) {
		throw CRYPTO_EXCEPTION_F("crypto_generichash_blake2b failed with error {}", result);
	}

	return output;
}

Hash Hasher::Blake2b(const std::vector<uint8_t>& key, const std::vector<uint8_t>& input)
{
    return Blake2b(key, input.data(), input.size());
}

Hash Hasher::Blake2b(const std::vector<uint8_t>& key, const uint8_t* input, const size_t len)
{
	Hash output;

	int result = crypto_generichash_blake2b(output.data(), output.size(), input, len, key.data(), key.size());
	if (result != 0) {
		throw CRYPTO_EXCEPTION_F("crypto_generichash_blake2b failed with error {}", result);
	}

	return output;
}

Hash Hasher::SHA256(const std::vector<uint8_t>& input)
{
    return SHA256(input.data(), input.size());
}

Hash Hasher::SHA256(const uint8_t* input, const size_t len)
{
    Hash result;

	CSHA256().Write(input, len).Finalize(result.data());

	return result;
}

CBigInteger<64> Hasher::SHA512(const std::vector<uint8_t>& input)
{
    return SHA512(input.data(), input.size());
}

CBigInteger<64> Hasher::SHA512(const uint8_t* input, const size_t len)
{
    CBigInteger<64> result;

	CSHA512().Write(input, len).Finalize(result.data());

	return result;
}

CBigInteger<20> Hasher::RipeMD160(const std::vector<uint8_t>& input)
{
    return RipeMD160(input.data(), input.size());
}

CBigInteger<20> Hasher::RipeMD160(const uint8_t* input, const size_t len)
{
    CBigInteger<20> result;

	CRIPEMD160().Write(input, len).Finalize(result.data());

	return result;
}

Hash Hasher::HMAC_SHA256(const std::vector<uint8_t>& key, const std::vector<uint8_t>& input)
{
    return HMAC_SHA256(key.data(), key.size(), input.data(), input.size());
}

Hash Hasher::HMAC_SHA256(const uint8_t* key, const size_t key_len, const uint8_t* data, const size_t data_len)
{
    Hash result;

	CHMAC_SHA256(key, key_len).Write(data, data_len).Finalize(result.data());

	return result;
}

CBigInteger<64> Hasher::HMAC_SHA512(const std::vector<uint8_t>& key, const std::vector<uint8_t>& input)
{
    return HMAC_SHA512(key, input.data(), input.size());
}

CBigInteger<64> Hasher::HMAC_SHA512(const std::vector<uint8_t>& key, const uint8_t* input, const size_t len)
{
	CBigInteger<64> result;

	CHMAC_SHA512(key.data(), key.size()).Write(input, len).Finalize(result.data());

	return result;
}

uint64_t Hasher::SipHash24(const uint64_t k0, const uint64_t k1, const std::vector<unsigned char>& data)
{
	const std::vector<uint64_t> key = { k0, k1 };

	return siphash24(&key[0], &data[0], data.size());
}