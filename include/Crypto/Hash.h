#pragma once

#include <Crypto/BigInteger.h>
#include <Common/Util/BitUtil.h>

typedef CBigInteger<32> Hash;

static const Hash ZERO_HASH = { Hash::ValueOf(0) };
static const int HASH_SIZE = 32;


namespace std
{
	template<>
	struct hash<Hash>
	{
		size_t operator()(const Hash& hash) const
		{
			return BitUtil::ConvertToU64(hash[0], hash[4], hash[8], hash[12], hash[16], hash[20], hash[24], hash[28]);
		}
	};
}