#pragma once

#include <cstdint>
#include <chrono>
#include <ctime>

#include "portable_endian.h"

#include <Common/Logger.h>

// This should theoretically be uint32_t for EDGEBITS of <= 31, but we're just verifying, so efficiency isn't critical.
typedef uint64_t word_t;

// The (even) length of the cycle to be found. a minimum of 12 is recommended
#define PROOFSIZE 42

enum verify_code { POW_OK, POW_HEADER_LENGTH, POW_TOO_BIG, POW_TOO_SMALL, POW_NON_MATCHING, POW_BRANCH, POW_DEAD_END, POW_SHORT_CYCLE, POW_UNBALANCED };
static const char* errstr[] = { "OK", "wrong header length", "edge too big", "edges not ascending", "endpoints don't match up", "branch in cycle", "cycle dead ends", "cycle too short", "edges not balanced" };

#ifndef EDGE_BLOCK_BITS
#define EDGE_BLOCK_BITS 6
#endif
#define EDGE_BLOCK_SIZE (1 << EDGE_BLOCK_BITS)
#define EDGE_BLOCK_MASK (EDGE_BLOCK_SIZE - 1)


// generalize siphash by using a quadruple of 64-bit keys,
class siphash_keys
{
public:
    uint64_t k0;
    uint64_t k1;
    uint64_t k2;
    uint64_t k3;

    siphash_keys(const char* keybuf)
    {
        k0 = htole64(((uint64_t*)keybuf)[0]);
        k1 = htole64(((uint64_t*)keybuf)[1]);
        k2 = htole64(((uint64_t*)keybuf)[2]);
        k3 = htole64(((uint64_t*)keybuf)[3]);
    }
};

template <int rotE = 21>
class siphash_state
{
public:
    uint64_t v0;
    uint64_t v1;
    uint64_t v2;
    uint64_t v3;

    siphash_state(const siphash_keys& sk)
    {
        v0 = sk.k0; v1 = sk.k1; v2 = sk.k2; v3 = sk.k3;
    }
    uint64_t xor_lanes()
    {
        return (v0 ^ v1) ^ (v2 ^ v3);
    }
    void xor_with(const siphash_state& x)
    {
        v0 ^= x.v0;
        v1 ^= x.v1;
        v2 ^= x.v2;
        v3 ^= x.v3;
    }
    static uint64_t rotl(uint64_t x, uint64_t b)
    {
        return (x << b) | (x >> (64 - b));
    }
    void sip_round()
    {
        v0 += v1; v2 += v3; v1 = rotl(v1, 13);
        v3 = rotl(v3, 16); v1 ^= v0; v3 ^= v2;
        v0 = rotl(v0, 32); v2 += v1; v0 += v3;
        v1 = rotl(v1, 17);   v3 = rotl(v3, rotE);
        v1 ^= v2; v3 ^= v0; v2 = rotl(v2, 32);
    }
    void hash24(const uint64_t nonce)
    {
        v3 ^= nonce;
        sip_round(); sip_round();
        v0 ^= nonce;
        v2 ^= 0xff;
        sip_round(); sip_round(); sip_round(); sip_round();
    }
};

// fills buffer with EDGE_BLOCK_SIZE siphash outputs for block containing edge in cuckaroo graph
// return siphash output for given edge
template <int rotE = 21>
static uint64_t sipblock(siphash_keys& keys, const word_t edge, uint64_t* buf)
{
    siphash_state<rotE> shs(keys);
    word_t edge0 = edge & ~EDGE_BLOCK_MASK;
    for (uint32_t i = 0; i < EDGE_BLOCK_SIZE; i++)
    {
        shs.hash24(edge0 + i);
        buf[i] = shs.xor_lanes();
    }

    const uint64_t last = buf[EDGE_BLOCK_MASK];
    for (uint32_t i = 0; i < EDGE_BLOCK_MASK; i++)
    {
        buf[i] ^= last;
    }

    return buf[edge & EDGE_BLOCK_MASK];
}