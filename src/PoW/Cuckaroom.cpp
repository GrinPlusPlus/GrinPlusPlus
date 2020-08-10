#include "Cuckaroom.h"
#include "Common.h"

// the main parameter is the number of bits in an edge index,
// i.e. the 2-log of the number of edges
#define EDGEBITS 29

// number of edges
#define NEDGES ((word_t)1 << EDGEBITS)
#define EDGEMASK ((word_t)NEDGES - 1)
#define NNODES NEDGES
// used to mask siphash output
#define NODEMASK ((word_t)NNODES - 1)

// verify that edges are ascending and form a cycle in header-generated graph
int verify_cuckaroom(const word_t edges[PROOFSIZE], siphash_keys& keys)
{
	word_t xorfrom = 0, xorto = 0;
	u64 sips[EDGE_BLOCK_SIZE];
	word_t from[PROOFSIZE], to[PROOFSIZE], visited[PROOFSIZE];

	for (u32 n = 0; n < PROOFSIZE; n++)
	{
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

bool Cuckaroom::Validate(const BlockHeader& blockHeader)
{
	const ProofOfWork& proofOfWork = blockHeader.GetProofOfWork();
	const std::vector<uint64_t> proofNonces = proofOfWork.GetProofNonces();
	if (proofNonces.size() != PROOFSIZE) {
		LOG_ERROR("Invalid proof size");
		return false;
	}

	const std::vector<unsigned char> preProofOfWork = blockHeader.GetPreProofOfWork();
	siphash_keys keys((const char*)Crypto::Blake2b(preProofOfWork).data());
	const int result = verify_cuckaroom((const word_t*)proofNonces.data(), keys);
	if (result != POW_OK) {
		LOG_ERROR_F("Failed with result: {}", errstr[result]);
	}

	return result == POW_OK;
}