#include "Cuckarood.h"

#include "cuckoo/cuckarood/cuckarood.hpp"

bool Cuckarood::Validate(const BlockHeader& blockHeader)
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

	const int result = verify((const word_t*)proofNonces.data(), keys);

	return result == POW_OK;
}