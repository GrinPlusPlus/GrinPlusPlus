#pragma once

#include <cstdint>
#include <chrono>
#include <ctime>

#include "siphash.hpp"

#include <Common/Logger.h>

// save some keystrokes since i'm a lazy typer
typedef uint32_t u32;
typedef uint64_t u64;

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

// fills buffer with EDGE_BLOCK_SIZE siphash outputs for block containing edge in cuckaroo graph
// return siphash output for given edge
template <int rotE = 21>
static u64 sipblock(siphash_keys& keys, const word_t edge, u64* buf)
{
    siphash_state<rotE> shs(keys);
    word_t edge0 = edge & ~EDGE_BLOCK_MASK;
    for (u32 i = 0; i < EDGE_BLOCK_SIZE; i++)
    {
        shs.hash24(edge0 + i);
        buf[i] = shs.xor_lanes();
    }

    const u64 last = buf[EDGE_BLOCK_MASK];
    for (u32 i = 0; i < EDGE_BLOCK_MASK; i++)
    {
        buf[i] ^= last;
    }

    return buf[edge & EDGE_BLOCK_MASK];
}