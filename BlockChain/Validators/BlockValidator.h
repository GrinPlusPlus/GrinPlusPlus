#pragma once

#include <Core/FullBlock.h>

// Forward Declarations
class BlindingFactor;
class ITransactionPool;
class ITxHashSet;

class BlockValidator
{
public:
	BlockValidator(ITransactionPool& transactionPool, ITxHashSet* pTxHashSet);

	bool IsBlockValid(const FullBlock& block, const BlindingFactor& previousKernelOffset) const;

private:
	bool VerifyKernelLockHeights(const FullBlock& block) const;
	bool VerifyCoinbase(const FullBlock& block) const;

	ITransactionPool& m_transactionPool;
	ITxHashSet* m_pTxHashSet;
};