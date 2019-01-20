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

	bool IsBlockValid(const FullBlock& block, const BlindingFactor& previousKernelOffset, const bool validateTransactionBody) const;

private:
	bool VerifyKernelLockHeights(const FullBlock& block) const;
	bool VerifyCoinbase(const FullBlock& block) const;
	bool VerifyKernelSums(const FullBlock& block, int64_t overage, const BlindingFactor& kernelOffset) const;

	ITransactionPool& m_transactionPool;
	ITxHashSet* m_pTxHashSet;
};