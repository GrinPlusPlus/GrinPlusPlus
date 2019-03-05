#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Common/ImportExport.h>
#include <Crypto/BigInteger.h>
#include <stdint.h>
#include <vector>
#include <memory>
#include <Crypto/Commitment.h>
#include <Crypto/RangeProof.h>
#include <Crypto/BlindingFactor.h>
#include <Crypto/Signature.h>
#include <Crypto/ProofMessage.h>
#include <Crypto/RewoundProof.h>
#include <Crypto/Hash.h>

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
	// Uses SHA256 to hash the given input into a 32 byte hash.
	//
	static CBigInteger<32> SHA256(const std::vector<unsigned char>& input);

	//
	// Uses RipeMD160 to hash the given input into a 20 byte hash.
	//
	static CBigInteger<20> RipeMD160(const std::vector<unsigned char>& input);

	//
	//
	//
	static CBigInteger<32> HMAC_SHA256(const std::vector<unsigned char>& key, const std::vector<unsigned char>& data);

	//
	//
	//
	static CBigInteger<64> HMAC_SHA512(const std::vector<unsigned char>& key, const std::vector<unsigned char>& data);

	//
	// Creates a pedersen commitment from a value with a zero blinding factor.
	//
	static std::unique_ptr<Commitment> CommitTransparent(const uint64_t value);

	//
	// Creates a pedersen commitment from a value with the supplied blinding factor.
	//
	static std::unique_ptr<Commitment> CommitBlinded(const uint64_t value, const BlindingFactor& blindingFactor);

	//
	// Adds the homomorphic pedersen commitments together.
	//
	static std::unique_ptr<Commitment> AddCommitments(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative);

	//
	// Takes a vector of blinding factors and calculates an additional blinding value that adds to zero.
	//
	static std::unique_ptr<BlindingFactor> AddBlindingFactors(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative);

	static std::unique_ptr<RangeProof> GenerateRangeProof(const uint64_t amount, const BlindingFactor& key, const CBigInteger<32>& nonce, const ProofMessage& proofMessage);

	static std::unique_ptr<RewoundProof> RewindRangeProof(const Commitment& commitment, const RangeProof& rangeProof, const CBigInteger<32>& nonce);

	//
	//
	//
	static bool VerifyRangeProofs(const std::vector<std::pair<Commitment, RangeProof>>& rangeProofs);

	//
	//
	//
	static bool VerifyKernelSignature(const Signature& signature, const Commitment& publicKey, const Hash& message);

	//
	//
	//
	static uint64_t SipHash24(const uint64_t k0, const uint64_t k1, const std::vector<unsigned char>& data);

	//
	// Encrypts the input with AES256 using the given key.
	//
	static std::vector<unsigned char> AES256_Encrypt(const std::vector<unsigned char>& input, const CBigInteger<32>& key, const CBigInteger<16>& iv);

	//
	// Decrypts the input with AES256 using the given key.
	//
	static std::vector<unsigned char> AES256_Decrypt(const std::vector<unsigned char>& ciphertext, const CBigInteger<32>& key, const CBigInteger<16>& iv);

	//
	// Uses Scrypt to hash the given input (usually the user's password) and salts the hash with the given salt.
	// The returned hash will be 64 bytes.
	//
	static CBigInteger<64> Scrypt(const std::vector<unsigned char>& input, const std::vector<unsigned char>& salt);

	//
	// Calculates the 33 byte public key from the 32 byte private key using curve secp256k1.
	//
	static std::unique_ptr<CBigInteger<33>> SECP256K1_CalculateCompressedPublicKey(const BlindingFactor& privateKey);

	static std::unique_ptr<CBigInteger<33>> AddPublicKeys(const std::vector<CBigInteger<33>>& publicKeys);

	static std::unique_ptr<Signature> CalculatePartialSignature(const BlindingFactor& secretKey, const BlindingFactor& secretNonce, const CBigInteger<33>& sumPubKeys, const CBigInteger<33>& sumPubNonces, const Hash& message);
};