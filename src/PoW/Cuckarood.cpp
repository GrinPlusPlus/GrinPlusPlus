#include "Cuckarood.h"
#include "Common.h"

#include <Crypto/Hasher.h>

// the main parameter is the number of bits in an edge index,
// i.e. the 2-log of the number of edges
#define EDGEBITS 29

// number of edges
#define NEDGES2 ((word_t)1 << EDGEBITS)
#define NEDGES1 (NEDGES2 / 2)
#define NNODES1 NEDGES1
// used to mask siphash output
#define NODE1MASK ((word_t)NNODES1 - 1)

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckarood(const word_t edges[PROOFSIZE], siphash_keys& keys)
{
    word_t xor0 = 0, xor1 = 0;
    uint64_t sips[EDGE_BLOCK_SIZE];
    word_t uvs[2 * PROOFSIZE];
    uint32_t ndir[2] = { 0, 0 };

    for (uint32_t n = 0; n < PROOFSIZE; n++) {
        uint32_t dir = edges[n] & 1;
        if (ndir[dir] >= PROOFSIZE / 2)
            return POW_UNBALANCED;
        if (edges[n] >= NEDGES2)
            return POW_TOO_BIG;
        if (n && edges[n] <= edges[n - 1])
            return POW_TOO_SMALL;
        uint64_t edge = sipblock<25>(keys, edges[n], sips);
        xor0 ^= uvs[4 * ndir[dir] + 2 * dir] = edge & NODE1MASK;
        // printf("%2d %8x\t", 4 * ndir[dir] + 2 * dir , edge        & NODE1MASK);
        xor1 ^= uvs[4 * ndir[dir] + 2 * dir + 1] = (edge >> 32) & NODE1MASK;
        // printf("%2d %8x\n", 4 * ndir[dir] + 2 * dir + 1 ,(edge >> 32) & NODE1MASK);
        ndir[dir]++;
    }
    if (xor0 | xor1)              // optional check for obviously bad proofs
        return POW_NON_MATCHING;
    uint32_t n = 0, i = 0, j;
    do {                        // follow cycle
        for (uint32_t k = ((j = i) % 4) ^ 2; k < 2 * PROOFSIZE; k += 4) {
            if (uvs[k] == uvs[i]) { // find reverse direction edge endpoint identical to one at i
                if (j != i)           // already found one before
                    return POW_BRANCH;
                j = k;
            }
        }
        if (j == i) return POW_DEAD_END;  // no matching endpoint
        i = j ^ 1;
        n++;
    } while (i != 0);           // must cycle back to start or we would have found branch
    return n == PROOFSIZE ? POW_OK : POW_SHORT_CYCLE;
}

bool Cuckarood::Validate(const BlockHeader& blockHeader)
{
	const ProofOfWork& proofOfWork = blockHeader.GetProofOfWork();
	const std::vector<uint64_t> proofNonces = proofOfWork.GetProofNonces();
	if (proofNonces.size() != PROOFSIZE) {
		LOG_ERROR("Invalid proof size");
		return false;
	}

    Hash prePoWHash = Hasher::Blake2b(blockHeader.GetPreProofOfWork());
    siphash_keys keys((const char*)prePoWHash.data());
	const int result = verify_cuckarood((const word_t*)proofNonces.data(), keys);
	if (result != POW_OK) {
		LOG_ERROR_F("Failed with result: {}", errstr[result]);
	}

	return result == POW_OK;
}