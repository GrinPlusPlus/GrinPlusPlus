#pragma once

#include <stdint.h>

namespace BitUtil
{
	static uint64_t FillOnesToRight(const uint64_t input)
	{
		uint64_t x = input;
		x = x | (x >> 1);
		x = x | (x >> 2);
		x = x | (x >> 4);
		x = x | (x >> 8);
		x = x | (x >> 16);
		x = x | (x >> 32);
		return x;
	}

	static uint8_t CountBitsSet(const uint64_t input)
	{
		uint64_t n = input;
		uint8_t count = 0;
		while (n)
		{
			count += n & 1;
			n >>= 1;
		}

		return count;
	}
}