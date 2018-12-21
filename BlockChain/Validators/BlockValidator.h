#pragma once

#include <Core/FullBlock.h>

// Forward Declarations
class BlindingFactor;
class ITxHashSet;

class BlockValidator
{
public:
	BlockValidator(ITxHashSet* pTxHashSet);

	bool IsBlockValid(const FullBlock& block, const BlindingFactor& previousKernelOffset) const;

private:
	bool VerifyKernelLockHeights(const FullBlock& block) const;
	bool VerifyCoinbase(const FullBlock& block) const;

	ITxHashSet* m_pTxHashSet;
};