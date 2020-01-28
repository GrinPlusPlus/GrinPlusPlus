#pragma once

#include <Crypto/BigInteger.h>
#include <Common/Util/BitUtil.h>
#include <Common/Util/HexUtil.h>

typedef CBigInteger<32> Hash;

//static constexpr Hash ZERO_HASH = { Hash::ValueOf(0) };
static constexpr int HASH_SIZE = 32;


class HASH
{
public:
	static inline const Hash ZERO = Hash::ValueOf(0);
	static std::string ShortHash(const Hash& hash) { return HexUtil::ConvertToHex(hash.GetData(), 6); }
};

#define ZERO_HASH HASH::ZERO

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