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

	static uint16_t ConvertToU16(const uint8_t byte1, const uint8_t byte2)
	{
		return (((uint16_t)byte1) << 8) | ((uint16_t)byte2);
	}

	static uint32_t ConvertToU32(const uint8_t byte1, const uint8_t byte2, const uint8_t byte3, const uint8_t byte4)
	{
		return ((((uint32_t)byte1) << 24) | (((uint32_t)byte2) << 16) | (((uint32_t)byte3) << 8) | ((uint32_t)byte4));
	}

	static uint64_t ConvertToU64(const uint8_t byte1, const uint8_t byte2, const uint8_t byte3, const uint8_t byte4, 
		const uint8_t byte5, const uint8_t byte6, const uint8_t byte7, const uint8_t byte8)
	{
		return ((((uint64_t)byte1) << 56) | (((uint64_t)byte2) << 48) | (((uint64_t)byte3) << 40) | ((uint64_t)byte4) << 32
			| ((uint64_t)byte5) << 24 | ((uint64_t)byte5) << 16 | ((uint64_t)byte5) << 8 | ((uint64_t)byte5));
	}
}