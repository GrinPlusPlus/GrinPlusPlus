#include "Cuckatoo.h"
#include "Common.h"

#include <Crypto/Hasher.h>

// generate edge endpoint in cuck(at)oo graph without partition bit
word_t sipnode(siphash_keys* keys, word_t edge, uint32_t uorv, const word_t edgeMask)
{
    siphash_state v(*keys);
    v.hash24(2 * edge + uorv);
    return v.xor_lanes() & edgeMask;
}

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckatoo(const word_t edges[PROOFSIZE], siphash_keys* keys, const uint8_t edgeBits)
{
    word_t uvs[2 * PROOFSIZE], xor0, xor1;
    xor0 = xor1 = (PROOFSIZE / 2) & 1;

    // number of edges
    const word_t numEdges = ((word_t)1 << edgeBits);

    // used to mask siphash output
    const word_t edgeMask = ((word_t)numEdges - 1);

    for (uint32_t n = 0; n < PROOFSIZE; n++)
    {
        if (edges[n] > edgeMask) {
            return POW_TOO_BIG;
        }

        if (n && edges[n] <= edges[n - 1]) {
            return POW_TOO_SMALL;
        }

        xor0 ^= uvs[2 * n] = sipnode(keys, edges[n], 0, edgeMask);
        xor1 ^= uvs[2 * n + 1] = sipnode(keys, edges[n], 1, edgeMask);
    }

    // optional check for obviously bad proofs
    if (xor0 | xor1) {
        return POW_NON_MATCHING;
    }

    uint32_t n = 0, i = 0, j;
    do
    {
        // follow cycle
        for (uint32_t k = j = i; (k = (k + 2) % (2 * PROOFSIZE)) != i; )
        {
            if (uvs[k] >> 1 == uvs[i] >> 1) {
                // find other edge endpoint matching one at i
                if (j != i) {
                    // already found one before
                    return POW_BRANCH;
                }

                j = k;
            }
        }

        if (j == i || uvs[j] == uvs[i]) {
            // no matching endpoint
            return POW_DEAD_END;
        }

        i = j ^ 1;
        n++;
    } while (i != 0);           // must cycle back to start or we would have found branch

    return n == PROOFSIZE ? POW_OK : POW_SHORT_CYCLE;
}

bool Cuckatoo::Validate(const BlockHeader& blockHeader)
{
	const ProofOfWork& proofOfWork = blockHeader.GetProofOfWork();
	const std::vector<uint64_t> proofNonces = proofOfWork.GetProofNonces();
	if (proofNonces.size() != PROOFSIZE) {
		return false;
	}

    Hash prePoWHash = Hasher::Blake2b(blockHeader.GetPreProofOfWork());
    siphash_keys keys((const char*)prePoWHash.data());
	const int result = verify_cuckatoo(proofNonces.data(), &keys, proofOfWork.GetEdgeBits());
	if (result != POW_OK) {
		LOG_ERROR_F("Failed with result: {}", errstr[result]);
	}

	return result == POW_OK;
}