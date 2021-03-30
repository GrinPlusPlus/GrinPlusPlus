#pragma once

#include <cstdint>
#include <Crypto/Models/BigInteger.h>

namespace KeyDefs
{
	static const uint32_t MINIMUM_HARDENED_INDEX = 2147483648;
	static const CBigInteger<32> SECP256K1_N = CBigInteger<32>::FromHex("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
	static const CBigInteger<32> BIG_INT_ZERO = CBigInteger<32>::ValueOf(0);
}