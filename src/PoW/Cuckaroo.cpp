#include "Cuckaroo.h"
#include "Common.h"

#include <Crypto/Hasher.h>

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckaroo(const uint64_t edges[PROOFSIZE], siphash_keys& keys, const uint8_t edgeBits)
{
    uint64_t xor0 = 0, xor1 = 0;
    uint64_t sips[EDGE_BLOCK_SIZE];
    uint64_t uvs[2 * PROOFSIZE];

    // number of edges
    const uint64_t numEdges = ((uint64_t)1 << edgeBits);

    // used to mask siphash output
    const uint64_t edgeMask = ((uint64_t)numEdges - 1);

    for (uint32_t n = 0; n < PROOFSIZE; n++)
    {
        if (edges[n] > edgeMask) {
            return POW_TOO_BIG;
        }

        if (n && edges[n] <= edges[n - 1]) {
            return POW_TOO_SMALL;
        }

        uint64_t edge = sipblock(keys, edges[n], sips);
        xor0 ^= uvs[2 * n] = edge & edgeMask;
        xor1 ^= uvs[2 * n + 1] = (edge >> 32) & edgeMask;
    }

    if (xor0 | xor1) {
        // optional check for obviously bad proofs
        return POW_NON_MATCHING;
    }

    uint32_t n = 0, i = 0, j;
    do // follow cycle
    {
        for (uint32_t k = j = i; (k = (k + 2) % (2 * PROOFSIZE)) != i; )
        {
            if (uvs[k] == uvs[i]) { // find other edge endpoint identical to one at i
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

bool Cuckaroo::Validate(const BlockHeader& blockHeader)
{
	const ProofOfWork& proofOfWork = blockHeader.GetProofOfWork();
	const std::vector<uint64_t> proofNonces = proofOfWork.GetProofNonces();
	if (proofNonces.size() != PROOFSIZE) {
		return false;
	}

    Hash prePoWHash = Hasher::Blake2b(blockHeader.GetPreProofOfWork());
    siphash_keys keys((const char*)prePoWHash.data());
	const int result = verify_cuckaroo(proofNonces.data(), keys, proofOfWork.GetEdgeBits());
	if (result != POW_OK) {
		LOG_ERROR_F("Failed with result: {}", errstr[result]);
	}

	return result == POW_OK;
}