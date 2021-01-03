#include "Cuckarooz.h"
#include "Common.h"

#include <Crypto/Hasher.h>

// the main parameter is the number of bits in an edge index,
// i.e. the 2-log of the number of edges
#define EDGEBITS 29

// number of edges
#define NEDGES ((word_t)1 << EDGEBITS)
#define EDGEMASK ((word_t)NEDGES - 1)
#define NNODES (2*NEDGES)
// used to mask siphash output
#define NODEMASK ((word_t)NNODES - 1)

// fills buffer with EDGE_BLOCK_SIZE siphash outputs for block containing edge in cuckaroo graph
// return siphash output for given edge
uint64_t cuckarooz_sipblock(siphash_keys& keys, const word_t edge, uint64_t* buf)
{
    siphash_state<> shs(keys);
    word_t edge0 = edge & ~EDGE_BLOCK_MASK;
    for (uint32_t i = 0; i < EDGE_BLOCK_SIZE; i++)
    {
        shs.hash24(edge0 + i);
        buf[i] = shs.xor_lanes();
    }

    for (uint32_t i = EDGE_BLOCK_MASK; i; i--)
    {
        buf[i - 1] ^= buf[i];
    }

    return buf[edge & EDGE_BLOCK_MASK];
}

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckarooz(const word_t edges[PROOFSIZE], siphash_keys& keys)
{
    word_t xoruv = 0;
    uint64_t sips[EDGE_BLOCK_SIZE];
    word_t uv[2 * PROOFSIZE];

    for (uint32_t n = 0; n < PROOFSIZE; n++)
    {
        if (edges[n] > EDGEMASK) {
            return POW_TOO_BIG;
        }

        if (n && edges[n] <= edges[n - 1]) {
            return POW_TOO_SMALL;
        }

        uint64_t edge = cuckarooz_sipblock(keys, edges[n], sips);
        xoruv ^= uv[2 * n] = edge & NODEMASK;
        xoruv ^= uv[2 * n + 1] = (edge >> 32) & NODEMASK;
    }

    if (xoruv) {
        // optional check for obviously bad proofs
        return POW_NON_MATCHING;
    }

    uint32_t n = 0, i = 0, j;
    do // follow cycle
    {
        for (uint32_t k = j = i; (k = (k + 1) % (2 * PROOFSIZE)) != i; )
        {
            if (uv[k] == uv[i]) { // find other edge endpoint identical to one at i
                if (j != i) {
                    // already found one before
                    return POW_BRANCH;
                }

                j = k;
            }
        }

        if (j == i) {
            // no matching endpoint
            return POW_DEAD_END;
        }

        i = j ^ 1;
        n++;
    } while (i != 0);           // must cycle back to start or we would have found branch
    return n == PROOFSIZE ? POW_OK : POW_SHORT_CYCLE;
}

bool Cuckarooz::Validate(const BlockHeader& blockHeader)
{
	const ProofOfWork& proofOfWork = blockHeader.GetProofOfWork();
	const std::vector<uint64_t> proofNonces = proofOfWork.GetProofNonces();
	if (proofNonces.size() != PROOFSIZE) {
		LOG_ERROR("Invalid proof size");
		return false;
	}

    Hash prePoWHash = Hasher::Blake2b(blockHeader.GetPreProofOfWork());
    siphash_keys keys((const char*)prePoWHash.data());
	const int result = verify_cuckarooz((const word_t*)proofNonces.data(), keys);
	if (result != POW_OK) {
		LOG_ERROR_F("Failed with result: {}", errstr[result]);
	}

	return result == POW_OK;
}