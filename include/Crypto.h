#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <BigInteger.h>
#include <stdint.h>
#include <vector>
#include <memory>
#include <Crypto/Commitment.h>
#include <Crypto/RangeProof.h>
#include <Crypto/BlindingFactor.h>
#include <Crypto/Signature.h>
#include <Hash.h>

#ifdef MW_CRYPTO
#define CRYPTO_API EXPORT
#else
#define CRYPTO_API IMPORT
#endif

//
// Exported class that serves as a lightweight, easy-to-use wrapper for secp256k1-zkp and other crypto dependencies.
//
class CRYPTO_API Crypto
{
public:
	//
	// Uses Blake2b to hash the given input into a 32 byte hash.
	//
	static CBigInteger<32> Blake2b(const std::vector<unsigned char>& input);

	//
	// Creates a pedersen commitment from a value with a zero blinding factor.
	//
	static std::unique_ptr<Commitment> CommitTransparent(const uint64_t value);

	//
	// Creates a pedersen commitment from a value with the supplied blinding factor.
	//
	static std::unique_ptr<Commitment> CommitBlinded(const uint64_t value, const BlindingFactor& blindingFactor);

	static std::unique_ptr<Commitment> AddCommitments(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative);

	static bool VerifyRangeProofs(const std::vector<Commitment>& commitments, const std::vector<RangeProof>& rangeProofs);
	static bool VerifyKernelSignature(const Signature& signature, const Commitment& publicKey, const Hash& message);

	static uint64_t SipHash24(const uint64_t k0, const uint64_t k1, const std::vector<unsigned char>& data);

	//
	// Encrypts the input with AES256 using the given key.
	//
	static std::vector<unsigned char> AES256_Encrypt(const std::vector<unsigned char>& input, const std::vector<unsigned char>& key);

	//
	// Decrypts the input with AES256 using the given key.
	//
	static std::vector<unsigned char> AES256_Decrypt(const std::vector<unsigned char>& encrypted, const std::vector<unsigned char>& key);

	//
	// Uses Scrypt to hash the given input (usually the user's password) and salts the hash with the given salt.
	// The returned hash will be 64 bytes.
	//
	static CBigInteger<64> Scrypt(const std::vector<unsigned char>& input, const std::vector<unsigned char>& salt);
};