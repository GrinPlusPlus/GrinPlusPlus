#pragma once

#include <Core/FullBlock.h>

// Forward Declarations
class BlindingFactor;
class ITransactionPool;
class ITxHashSet;

class BlockValidator
{
public:
	BlockValidator(const ITransactionPool& transactionPool, const ITxHashSet* pTxHashSet);

	bool IsBlockValid(const FullBlock& block, const BlindingFactor& previousKernelOffset, const bool validateSelfConsistent) const;
	bool IsBlockSelfConsistent(const FullBlock& block) const;

private:
	bool VerifyKernelLockHeights(const FullBlock& block) const;
	bool VerifyCoinbase(const FullBlock& block) const;
	bool VerifyKernelSums(const FullBlock& block, int64_t overage, const BlindingFactor& kernelOffset) const;

	const ITransactionPool& m_transactionPool;
	const ITxHashSet* m_pTxHashSet;
};