/*
*  siphash.h
*
*  Created by Masatoshi Teruya on 13/08/08.
*  Copyright 2013 Masatoshi Teruya. All rights reserved.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to
*  deal in the Software without restriction, including without limitation the
*  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
*  sell copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*  IN THE SOFTWARE.
*/

#ifndef ___SIPHASH_H___
#define ___SIPHASH_H___

//#include <unistd.h>
#include <stdint.h>
#include <limits.h>


#define SIPHASH_BIT_ROTL(b,n)  ((b<<n)|(b>>((sizeof(b)*CHAR_BIT)-n)))

#define SIPHASH_ROUND(s0,s1,s2,s3) {\
    s0 += s1; \
    s2 += s3; \
    s1 = SIPHASH_BIT_ROTL(s1,13); \
    s3 = SIPHASH_BIT_ROTL(s3,16); \
    s1 ^= s0; \
    s3 ^= s2; \
    s0 = SIPHASH_BIT_ROTL(s0,32); \
    s2 += s1; \
    s0 += s3; \
    s1 = SIPHASH_BIT_ROTL(s1,17); \
    s3 = SIPHASH_BIT_ROTL(s3,21); \
    s1 ^= s2; \
    s3 ^= s0; \
    s2 = SIPHASH_BIT_ROTL(s2,32); \
}

static inline uint64_t siphash(const uint8_t rblk, const uint8_t rfin,
	const uint64_t key[2], const void *src,
	size_t len)
{
	uint8_t *ptr = (uint8_t*)src;
	size_t byte64t = sizeof(uint64_t);
	// multiply by 8
	size_t w = (len / 8) << 3;

	// initialize
	// v0 = k0 ⊕ 736f6d6570736575
	uint64_t v0 = key[0] ^ 0x736f6d6570736575ULL;
	// v1 = k1 ⊕ 646f72616e646f6d
	uint64_t v1 = key[1] ^ 0x646f72616e646f6dULL;
	// v2 = k0 ⊕ 6c7967656e657261
	uint64_t v2 = key[0] ^ 0x6c7967656e657261ULL;
	// v3 = k1 ⊕ 7465646279746573
	uint64_t v3 = key[1] ^ 0x7465646279746573ULL;
	uint64_t mi = 0;
	size_t i = 0;
	uint8_t k = 0;

	// compression
	for (i = 0; i < w; i += byte64t) {
		mi = *(uint64_t*)(ptr + i);
		// v3 ⊕ = mi
		v3 ^= mi;
		// SipHash-c-d: c is the number of rounds per message block
		for (k = 0; k < rblk; k++) {
			SIPHASH_ROUND(v0, v1, v2, v3);
		}
		// v0 ⊕ = mi
		v0 ^= mi;
	}

	// ending with a byte encoding the positive integer b mod 256.
	mi = (uint64_t)len << 56;
	ptr += i;
	switch (len - i) {
	case 7: mi |= (uint64_t)ptr[6] << 48;
	case 6: mi |= (uint64_t)ptr[5] << 40;
	case 5: mi |= (uint64_t)ptr[4] << 32;
	case 4: mi |= (uint64_t)ptr[3] << 24;
	case 3: mi |= (uint64_t)ptr[2] << 16;
	case 2: mi |= (uint64_t)ptr[1] << 8;
	case 1: mi |= (uint64_t)*ptr;
	}
	v3 ^= mi;
	// SipHash-c-d: c is the number of rounds per message block
	for (k = 0; k < rblk; k++) {
		SIPHASH_ROUND(v0, v1, v2, v3);
	}
	v0 ^= mi;

	// finalization
	// v2 ⊕ = ff
	v2 ^= 0xff;
	// SipHash-c-d: d is the number of finalization rounds
	for (k = 0; k < rfin; k++) {
		SIPHASH_ROUND(v0, v1, v2, v3);
	}

	return v0 ^ v1 ^ v2 ^ v3;
}

#undef SIPHASH_BIT_ROTL
#undef SIPHASH_ROUND

#define siphash24(key,src,len)  siphash(2,4,key,src,len)
#define siphash48(key,src,len)  siphash(4,8,key,src,len)

#endif