#pragma once

#include "../ChainState.h"

#include <Core/FullBlock.h>

// Forward Declarations
class BlindingFactor;

class BlockValidator
{
public:
	BlockValidator(ChainState& chainState);

	bool IsBlockValid(const FullBlock& block) const;

private:
	bool IsAlreadyValidated(const FullBlock& block) const;
	bool IsSelfConsistent(const FullBlock& block, const BlindingFactor& previousKernelOffset) const;
	bool VerifyKernelLockHeights(const FullBlock& block) const;
	bool VerifyCoinbase(const FullBlock& block) const;

	ChainState& m_chainState;
};