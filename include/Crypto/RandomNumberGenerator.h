#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <BigInteger.h>
#include <vector>

#ifdef MW_CRYPTO
#define CRYPTO_API EXPORT
#else
#define CRYPTO_API IMPORT
#endif

//
// A wrapper around a third-party CSPRNG random number generator to generate cryptographically-secure random CBigIntegers.
// TODO: Create a dynamically-sized big integer to allow any sized random number.
//
class CRYPTO_API RandomNumberGenerator
{
public:
	//
	// Generates a cryptographically-strong pseudo-random number using entropy from a software algorithm.
	//
	static CBigInteger<32> GeneratePseudoRandomNumber(const CBigInteger<32>& minimum, const CBigInteger<32>& maximum);

private:
	static uint8_t DetermineNumberOfRandomBytesNeeded(const CBigInteger<32>& differenceBetweenMaximumAndMinimum);
	static std::vector<unsigned char> GeneratePseudoRandomBytes(const uint8_t numBytes);
	static CBigInteger<32> ConvertRandomBytesToBigInteger(const std::vector<unsigned char>& randomBytes);
};