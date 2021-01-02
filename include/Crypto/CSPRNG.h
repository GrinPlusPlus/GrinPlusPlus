#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/Secure.h>
#include <Crypto/BigInteger.h>
#include <vector>

#define CRYPTO_API

//
// A wrapper around a third-party CSPRNG random number generator to generate cryptographically-secure random numbers.
//
class CRYPTO_API CSPRNG
{
public:
	//
	// Generates a cryptographically-strong pseudo-random 32-byte number.
	//
	static CBigInteger<32> GenerateRandom32();

	static SecureVector GenerateRandomBytes(const size_t numBytes);

	static uint64_t GenerateRandom(const uint64_t minimum, const uint64_t maximum);
};