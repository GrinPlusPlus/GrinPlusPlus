// Cuck(at)oo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2019 John Tromp

#include <stdint.h> // for types uint32_t,uint64_t
#include <string.h> // for functions strlen, memset
#include <stdarg.h>
#include <stdio.h> 
#include <chrono>
#include <ctime>

#include "../common.h"

#ifndef EDGE_BLOCK_BITS
#define EDGE_BLOCK_BITS 6
#endif
#define EDGE_BLOCK_SIZE (1 << EDGE_BLOCK_BITS)
#define EDGE_BLOCK_MASK (EDGE_BLOCK_SIZE - 1)

// the main parameter is the number of bits in an edge index,
// i.e. the 2-log of the number of edges
#define EDGEBITS 29

// number of edges
#define NEDGES2 ((word_t)1 << EDGEBITS)
#define NEDGES1 (NEDGES2 / 2)
#define NNODES1 NEDGES1
// used to mask siphash output
#define NODE1MASK ((word_t)NNODES1 - 1)

// fills buffer with EDGE_BLOCK_SIZE siphash outputs for block containing edge in cuckaroo graph
// return siphash output for given edge
static u64 sipblock(siphash_keys &keys, const word_t edge, u64 *buf) {
    siphash_state<25> shs(keys);
    word_t edge0 = edge & ~EDGE_BLOCK_MASK;
    for (u32 i=0; i < EDGE_BLOCK_SIZE; i++) {
        shs.hash24(edge0 + i);
        buf[i] = shs.xor_lanes();
    }
    const u64 last = buf[EDGE_BLOCK_MASK];
    for (u32 i=0; i < EDGE_BLOCK_MASK; i++)
        buf[i] ^= last;
    return buf[edge & EDGE_BLOCK_MASK];
}

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckarood(const word_t edges[PROOFSIZE], siphash_keys &keys) {
  word_t xor0 = 0, xor1 = 0;
  u64 sips[EDGE_BLOCK_SIZE];
  word_t uvs[2*PROOFSIZE];
  u32 ndir[2] = { 0, 0 };

  for (u32 n = 0; n < PROOFSIZE; n++) {
    u32 dir = edges[n] & 1;
    if (ndir[dir] >= PROOFSIZE / 2)
      return POW_UNBALANCED;
    if (edges[n] >= NEDGES2)
      return POW_TOO_BIG;
    if (n && edges[n] <= edges[n-1])
      return POW_TOO_SMALL;
    u64 edge = sipblock(keys, edges[n], sips);
    xor0 ^= uvs[4 * ndir[dir] + 2 * dir    ] =  edge        & NODE1MASK;
    // printf("%2d %8x\t", 4 * ndir[dir] + 2 * dir , edge        & NODE1MASK);
    xor1 ^= uvs[4 * ndir[dir] + 2 * dir + 1] = (edge >> 32) & NODE1MASK;
    // printf("%2d %8x\n", 4 * ndir[dir] + 2 * dir + 1 ,(edge >> 32) & NODE1MASK);
    ndir[dir]++;
  }
  if (xor0 | xor1)              // optional check for obviously bad proofs
    return POW_NON_MATCHING;
  u32 n = 0, i = 0, j;
  do {                        // follow cycle
    for (u32 k = ((j = i) % 4) ^ 2; k < 2*PROOFSIZE; k += 4) {
      if (uvs[k] == uvs[i]) { // find reverse direction edge endpoint identical to one at i
        if (j != i)           // already found one before
          return POW_BRANCH;
        j = k;
      }
    }
    if (j == i) return POW_DEAD_END;  // no matching endpoint
    i = j^1;
    n++;
  } while (i != 0);           // must cycle back to start or we would have found branch
  return n == PROOFSIZE ? POW_OK : POW_SHORT_CYCLE;
}