#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <stdint.h>

//
// A header-only utility for determining and changing endianness of data.
//
class EndianHelper
{
public:
	static bool IsBigEndian()
	{
		union 
		{
			uint32_t i;
			char c[4];
		} bint = { 0x01020304 };

		return bint.c[0] == 1;
	}

	// In Visual Studio, _byteswap_ushort could be used.
	static uint16_t changeEndianness16(const uint16_t val)
	{
		return (val << 8) |          // left-shift always fills with zeros
			((val >> 8) & 0x00ff); // right-shift sign-extends, so force to zero
	}

	// In Visual Studio, _byteswap_ulong could be used.
	static uint32_t changeEndianness32(const uint32_t val)
	{
		return (val << 24) |
			((val << 8) & 0x00ff0000) |
			((val >> 8) & 0x0000ff00) |
			((val >> 24) & 0x000000ff);
	}

	static uint16_t GetBigEndian16(const uint16_t val)
	{
		if (IsBigEndian())
		{
			return val;
		}

		return changeEndianness16(val);
	}

	static uint32_t GetBigEndian32(const uint32_t val)
	{
		if (IsBigEndian())
		{
			return val;
		}

		return changeEndianness32(val);
	}
};