// Cuck(at)oo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2019 John Tromp

#include <string.h> // for functions strlen, memset
#include <stdarg.h>
#include <stdio.h>

#include "../common.h"

#ifndef MAX_SOLS
#define MAX_SOLS 4
#endif

#ifndef EDGE_BLOCK_BITS
#define EDGE_BLOCK_BITS 6
#endif
#define EDGE_BLOCK_SIZE (1 << EDGE_BLOCK_BITS)
#define EDGE_BLOCK_MASK (EDGE_BLOCK_SIZE - 1)

// proof-of-work parameters
#ifndef PROOFSIZE
// The (even) length of the cycle to be found. a minimum of 12 is recommended
#define PROOFSIZE 42
#endif

// This should theoretically be uint32_t for EDGEBITS of <= 30, but we're just verifying, so efficiency isn't critical.
typedef uint64_t word_t;

#define MAX_NAME_LEN 256

// fills buffer with EDGE_BLOCK_SIZE siphash outputs for block containing edge in cuckaroo graph
// return siphash output for given edge
static u64 sipblock(siphash_keys &keys, const word_t edge, u64 *buf) {
  siphash_state shs(keys);
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
int verify(const word_t edges[PROOFSIZE], siphash_keys &keys, const uint8_t edgeBits) {
  word_t xor0 = 0, xor1 = 0;
  u64 sips[EDGE_BLOCK_SIZE];
  word_t uvs[2*PROOFSIZE];

  // number of edges
  const word_t numEdges = ((word_t)1 << edgeBits);

  // used to mask siphash output
  const word_t edgeMask = ((word_t)numEdges - 1);

  for (u32 n = 0; n < PROOFSIZE; n++) {
    if (edges[n] > edgeMask)
      return POW_TOO_BIG;
    if (n && edges[n] <= edges[n-1])
      return POW_TOO_SMALL;
    u64 edge = sipblock(keys, edges[n], sips);
    xor0 ^= uvs[2*n  ] = edge & edgeMask;
    xor1 ^= uvs[2*n+1] = (edge >> 32) & edgeMask;
  }
  if (xor0 | xor1)              // optional check for obviously bad proofs
    return POW_NON_MATCHING;
  u32 n = 0, i = 0, j;
  do {                        // follow cycle
    for (u32 k = j = i; (k = (k+2) % (2*PROOFSIZE)) != i; ) {
      if (uvs[k] == uvs[i]) { // find other edge endpoint identical to one at i
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