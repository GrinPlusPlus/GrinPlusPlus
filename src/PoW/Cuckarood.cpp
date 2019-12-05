#include "Cuckarood.h"

#include "cuckoo/cuckarood/cuckarood.hpp"

#include <Infrastructure/Logger.h>

bool Cuckarood::Validate(const BlockHeader& blockHeader)
{
	const std::vector<unsigned char> preProofOfWork = blockHeader.GetPreProofOfWork();

	siphash_keys keys;
	setheader((const char*)preProofOfWork.data(), (uint32_t)preProofOfWork.size(), &keys);

	const ProofOfWork& proofOfWork = blockHeader.GetProofOfWork();
	const std::vector<uint64_t> proofNonces = proofOfWork.GetProofNonces();
	if (proofNonces.size() != PROOFSIZE)
	{
		LOG_ERROR("Invalid proof size");
		return false;
	}

	const int result = verify((const word_t*)proofNonces.data(), keys);
	if (result != POW_OK)
	{
		LOG_ERROR_F("Failed with result: {}", result);
	}

	return result == POW_OK;
}