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

	const int result = verify_cuckaroo(proofNonces.data(), keys, proofOfWork.GetEdgeBits());

	return result == POW_OK;
}