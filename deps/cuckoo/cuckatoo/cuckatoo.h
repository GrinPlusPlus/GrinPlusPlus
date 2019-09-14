// Cuck(at)oo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2019 John Tromp

#include <string.h> // for functions strlen, memset
#include <stdarg.h>
#include <stdio.h>

#include "../common.h"

#ifndef MAX_SOLS
#define MAX_SOLS 4
#endif

// proof-of-work parameters
#ifndef PROOFSIZE
// The (even) length of the cycle to be found. a minimum of 12 is recommended
#define PROOFSIZE 42
#endif

// This should theoretically be uint32_t for EDGEBITS of <= 31, but we're just verifying, so efficiency isn't critical.
typedef uint64_t word_t;

#define MAX_NAME_LEN 256

// generate edge endpoint in cuck(at)oo graph without partition bit
word_t sipnode(siphash_keys *keys, word_t edge, u32 uorv, const word_t edgeMask) {
  return keys->siphash24(2*edge + uorv) & edgeMask;
}

// verify that edges are ascending and form a cycle in header-generated graph
int verify(const word_t edges[PROOFSIZE], siphash_keys *keys, const uint8_t edgeBits) {
  word_t uvs[2*PROOFSIZE], xor0, xor1;
  xor0 = xor1 = (PROOFSIZE/2) & 1;

  // number of edges
  const word_t numEdges = ((word_t)1 << edgeBits);

  // used to mask siphash output
  const word_t edgeMask = ((word_t)numEdges - 1);

  for (u32 n = 0; n < PROOFSIZE; n++) {
    if (edges[n] > edgeMask)
      return POW_TOO_BIG;
    if (n && edges[n] <= edges[n-1])
      return POW_TOO_SMALL;
    xor0 ^= uvs[2*n  ] = sipnode(keys, edges[n], 0, edgeMask);
    xor1 ^= uvs[2*n+1] = sipnode(keys, edges[n], 1, edgeMask);
  }
  if (xor0|xor1)              // optional check for obviously bad proofs
    return POW_NON_MATCHING;
  u32 n = 0, i = 0, j;
  do {                        // follow cycle
    for (u32 k = j = i; (k = (k+2) % (2*PROOFSIZE)) != i; ) {
      if (uvs[k]>>1 == uvs[i]>>1) { // find other edge endpoint matching one at i
        if (j != i)           // already found one before
          return POW_BRANCH;
        j = k;
      }
    }
    if (j == i || uvs[j] == uvs[i])
      return POW_DEAD_END;  // no matching endpoint
    i = j^1;
    n++;
  } while (i != 0);           // must cycle back to start or we would have found branch
  return n == PROOFSIZE ? POW_OK : POW_SHORT_CYCLE;
}