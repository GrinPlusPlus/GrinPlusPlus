#include "Cuckaroo.h"

#include "cuckoo/cuckaroo/cuckaroo.hpp"

bool Cuckaroo::Validate(const BlockHeader& blockHeader)
{
	const std::vector<unsigned char> preProofOfWork = blockHeader.GetPreProofOfWork();

	siphash_keys keys;
	setheader((const char*)preProofOfWork.data(), (uint32_t)preProofOfWork.size(), &keys);

	const ProofOfWork& proofOfWork = blockHeader.GetProofOfWork();
	const std::vector<uint64_t> proofNonces = proofOfWork.GetProofNonces();
	if (proofNonces.size() != PROOFSIZE)
	{
		return false;
	}

	//uint32_t nonces[PROOFSIZE];
	//for (size_t i = 0; i < PROOFSIZE; i++)
	//{
	//	if (proofNonces[i] > UINT32_MAX)
	//	{
	//		return false;
	//	}

	//	nonces[i] = (uint32_t)proofNonces[i];
	//}

	const int result = verify(proofNonces.data(), keys, proofOfWork.GetEdgeBits());

	return result == POW_OK;
}