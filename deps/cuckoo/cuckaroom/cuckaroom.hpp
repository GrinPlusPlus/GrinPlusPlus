// Cuckaroom Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2020 John Tromp

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
#define NEDGES ((word_t)1 << EDGEBITS)
#define EDGEMASK ((word_t)NEDGES - 1)
#define NNODES NEDGES
// used to mask siphash output
#define NODEMASK ((word_t)NNODES - 1)

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