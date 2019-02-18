#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Common/ImportExport.h>
#include <Crypto/BigInteger.h>
#include <vector>

#ifdef MW_CRYPTO
#define CRYPTO_API EXPORT
#else
#define CRYPTO_API IMPORT
#endif

//
// A wrapper around a third-party CSPRNG random number generator to generate cryptographically-secure random numbers.
//
class CRYPTO_API RandomNumberGenerator
{
public:
	//
	// Generates a cryptographically-strong pseudo-random 32-byte number.
	// TODO: Create a dynamically-sized big integer to allow any sized random number.
	//
	static CBigInteger<32> GenerateRandom32();

	static std::vector<unsigned char> GenerateRandomBytes(const size_t numBytes);

	static uint64_t GenerateRandom(const uint64_t minimum, const uint64_t maximum);
};