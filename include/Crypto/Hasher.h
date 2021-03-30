#pragma once

#include <Crypto/Models/Hash.h>

class Hasher
{
public:
    //
	// Uses Blake2b to hash the given input into a 32 byte hash.
	//
	static Hash Blake2b(const std::vector<uint8_t>& input);

    //
	// Uses Blake2b to hash the given input into a 32 byte hash.
	//
	static Hash Blake2b(const uint8_t* input, const size_t len);

	//
	// Uses Blake2b to hash the given input into a 32 byte hash using a key.
	//
	static Hash Blake2b(
		const std::vector<uint8_t>& key,
		const std::vector<uint8_t>& input
	);

	//
	// Uses Blake2b to hash the given input into a 32 byte hash using a key.
	//
	static Hash Blake2b(
		const std::vector<uint8_t>& key,
        const uint8_t* input,
        const size_t len
	);

	//
	// Uses SHA256 to hash the given input into a 32 byte hash.
	//
	static Hash SHA256(const std::vector<uint8_t>& input);

	//
	// Uses SHA256 to hash the given input into a 32 byte hash.
	//
	static Hash SHA256(const uint8_t* input, const size_t len);

	//
	// Uses SHA512 to hash the given input into a 64 byte hash.
	//
	static CBigInteger<64> SHA512(const std::vector<uint8_t>& input);

	//
	// Uses SHA512 to hash the given input into a 64 byte hash.
	//
	static CBigInteger<64> SHA512(const uint8_t* input, const size_t len);

	//
	// Uses RipeMD160 to hash the given input into a 20 byte hash.
	//
	static CBigInteger<20> RipeMD160(const std::vector<uint8_t>& input);

	//
	// Uses RipeMD160 to hash the given input into a 20 byte hash.
	//
	static CBigInteger<20> RipeMD160(const uint8_t* input, const size_t len);

	//
	//
	//
	static Hash HMAC_SHA256(
		const std::vector<uint8_t>& key,
		const std::vector<uint8_t>& data
	);

	//
	//
	//
	static Hash HMAC_SHA256(
		const uint8_t* key,
		const size_t key_len,
		const uint8_t* data,
		const size_t data_len
	);

	//
	//
	//
	static CBigInteger<64> HMAC_SHA512(
		const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data
	);

	//
	//
	//
	static CBigInteger<64> HMAC_SHA512(
		const std::vector<uint8_t>& key,
		const uint8_t* data,
		const size_t len
	);

	//
	//
	//
	static uint64_t SipHash24(
		const uint64_t k0,
		const uint64_t k1,
		const std::vector<unsigned char>& data
	);
};