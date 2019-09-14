#pragma once

#include <stdint.h>
#include <Crypto/BigInteger.h>

namespace KeyDefs
{
	static const uint32_t MINIMUM_HARDENED_INDEX = 2147483648;
	static const CBigInteger<32> SECP256K1_N = CBigInteger<32>::FromHex("0xFFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141");
	static const CBigInteger<32> BIG_INT_ZERO = CBigInteger<32>::ValueOf(0);
}