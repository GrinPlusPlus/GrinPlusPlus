// Cuckaroom Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2020 John Tromp

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
#define NEDGES ((word_t)1 << EDGEBITS)
#define EDGEMASK ((word_t)NEDGES - 1)
#define NNODES NEDGES
// used to mask siphash output
#define NODEMASK ((word_t)NNODES - 1)

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
static u64 sipblock(siphash_keys& keys, const word_t edge, u64* buf) {
	siphash_state<> shs(keys);
	word_t edge0 = edge & ~EDGE_BLOCK_MASK;
	for (u32 i = 0; i < EDGE_BLOCK_SIZE; i++) {
		shs.hash24(edge0 + i);
		buf[i] = shs.xor_lanes();
	}
	for (u32 i = EDGE_BLOCK_MASK; i; i--)
		buf[i - 1] ^= buf[i];
	return buf[edge & EDGE_BLOCK_MASK];
}

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckaroom(const word_t edges[PROOFSIZE], siphash_keys& keys) {
	word_t xorfrom = 0, xorto = 0;
	u64 sips[EDGE_BLOCK_SIZE];
	word_t from[PROOFSIZE], to[PROOFSIZE], visited[PROOFSIZE];

	for (u32 n = 0; n < PROOFSIZE; n++) {
		if (edges[n] > EDGEMASK)
			return POW_TOO_BIG;
		if (n && edges[n] <= edges[n - 1])
			return POW_TOO_SMALL;
		u64 edge = sipblock(keys, edges[n], sips);
		xorfrom ^= from[n] = edge & EDGEMASK;
		xorto ^= to[n] = (edge >> 32) & EDGEMASK;
		visited[n] = false;
	}
	if (xorfrom != xorto)              // optional check for obviously bad proofs
		return POW_NON_MATCHING;
	u32 n = 0, i = 0;
	do {                        // follow cycle
		if (visited[i])
			return POW_BRANCH;
		visited[i] = true;
		u32 nexti;
		for (nexti = 0; from[nexti] != to[i]; ) // find outgoing edge meeting incoming edge i
			if (++nexti == PROOFSIZE)
				return POW_DEAD_END;
		i = nexti;
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