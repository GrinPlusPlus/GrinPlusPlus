// Cuck(at)oo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2019 John Tromp

#include <stdint.h> // for types uint32_t,uint64_t
#include <string.h> // for functions strlen, memset
#include <stdarg.h>
#include <stdio.h> 
#include <chrono>
#include <ctime>

#include "../common.h"

// save some keystrokes since i'm a lazy typer
typedef uint32_t u32;
typedef uint64_t u64;

#ifndef MAX_SOLS
#define MAX_SOLS 4
#endif

#ifndef EDGE_BLOCK_BITS
#define EDGE_BLOCK_BITS 6
#endif
#define EDGE_BLOCK_SIZE (1 << EDGE_BLOCK_BITS)
#define EDGE_BLOCK_MASK (EDGE_BLOCK_SIZE - 1)

// proof-of-work parameters
#ifndef EDGEBITS
// the main parameter is the number of bits in an edge index,
// i.e. the 2-log of the number of edges
#define EDGEBITS 29
#endif
#ifndef PROOFSIZE
// the next most important parameter is the (even) length
// of the cycle to be found. a minimum of 12 is recommended
#define PROOFSIZE 42
#endif

// This should theoretically be uint32_t for EDGEBITS of <= 30, but we're just verifying, so efficiency isn't critical.
typedef uint64_t word_t;

// number of edges
#define NEDGES2 ((word_t)1 << EDGEBITS)
#define NEDGES1 (NEDGES2 / 2)
#define NNODES1 NEDGES1
#define NNODES2 NEDGES2
// used to mask siphash output
#define NODE1MASK ((word_t)NNODES1 - 1)

// Common Solver parameters, to return to caller
struct SolverParams {
	u32 nthreads = 0;
	u32 ntrims = 0;
	bool showcycle;
	bool allrounds;
	bool mutate_nonce = 1;
	bool cpuload = 1;

	// Common cuda params
	u32 device = 0;

	// Cuda-lean specific params
	u32 blocks = 0;
	u32 tpb = 0;

	// Cuda-mean specific params
	u32 expand = 0;
	u32 genablocks = 0;
	u32 genatpb = 0;
	u32 genbtpb = 0;
	u32 trimtpb = 0;
	u32 tailtpb = 0;
	u32 recoverblocks = 0;
	u32 recovertpb = 0;
};

// Solutions result structs to be instantiated by caller,
// and filled by solver if desired
struct Solution {
 u64 id = 0;
 u64 nonce = 0;
 u64 proof[PROOFSIZE];
};

struct SolverSolutions {
 u32 edge_bits = 0;
 u32 num_sols = 0;
 Solution sols[MAX_SOLS];
};

#define MAX_NAME_LEN 256

// last error reason, to be picked up by stats
// to be returned to caller
char LAST_ERROR_REASON[MAX_NAME_LEN];

// Solver statistics, to be instantiated by caller
// and filled by solver if desired
struct SolverStats {
	u32 device_id = 0;
	u32 edge_bits = 0;
	char plugin_name[MAX_NAME_LEN]; // will be filled in caller-side
	char device_name[MAX_NAME_LEN];
	bool has_errored = false;
	char error_reason[MAX_NAME_LEN];
	u32 iterations = 0;
	u64 last_start_time = 0;
	u64 last_end_time = 0;
	u64 last_solution_time = 0;
};

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
int verify(const word_t edges[PROOFSIZE], siphash_keys &keys) {
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

/////////////////////////////////////////////////////////////////
// Declarations to make it easier for callers to link as required
/////////////////////////////////////////////////////////////////

#ifndef C_CALL_CONVENTION
#define C_CALL_CONVENTION 0
#endif

// convention to prepend to called functions
#if C_CALL_CONVENTION
#define CALL_CONVENTION extern "C"
#else
#define CALL_CONVENTION
#endif

// Ability to squash printf output at compile time, if desired
#ifndef SQUASH_OUTPUT
#define SQUASH_OUTPUT 0
#endif
//////////////////////////////////////////////////////////////////
// END caller QOL
//////////////////////////////////////////////////////////////////
