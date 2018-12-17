#pragma once

#include <Core/BlockHeader.h>

class PoWValidator
{
public:
	bool IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const;

private:
	uint64_t GetMaximumDifficulty(const ProofOfWork& proofOfWork) const;
};