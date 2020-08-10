#include "Cuckarooz.h"
#include "Common.h"

// the main parameter is the number of bits in an edge index,
// i.e. the 2-log of the number of edges
#define EDGEBITS 29

// number of edges
#define NEDGES ((word_t)1 << EDGEBITS)
#define EDGEMASK ((word_t)NEDGES - 1)
#define NNODES (2*NEDGES)
// used to mask siphash output
#define NODEMASK ((word_t)NNODES - 1)

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckarooz(const word_t edges[PROOFSIZE], siphash_keys& keys)
{
    word_t xoruv = 0;
    u64 sips[EDGE_BLOCK_SIZE];
    word_t uv[2 * PROOFSIZE];

    for (u32 n = 0; n < PROOFSIZE; n++)
    {
        if (edges[n] > EDGEMASK) {
            return POW_TOO_BIG;
        }

        if (n && edges[n] <= edges[n - 1]) {
            return POW_TOO_SMALL;
        }

        u64 edge = sipblock(keys, edges[n], sips);
        xoruv ^= uv[2 * n] = edge & NODEMASK;
        xoruv ^= uv[2 * n + 1] = (edge >> 32) & NODEMASK;
    }

    if (xoruv) {
        // optional check for obviously bad proofs
        return POW_NON_MATCHING;
    }

    u32 n = 0, i = 0, j;
    do // follow cycle
    {
        for (u32 k = j = i; (k = (k + 1) % (2 * PROOFSIZE)) != i; )
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

	const std::vector<unsigned char> preProofOfWork = blockHeader.GetPreProofOfWork();
	siphash_keys keys((const char*)Crypto::Blake2b(preProofOfWork).data());
	const int result = verify_cuckarooz((const word_t*)proofNonces.data(), keys);
	if (result != POW_OK) {
		LOG_ERROR_F("Failed with result: {}", errstr[result]);
	}

	return result == POW_OK;
}