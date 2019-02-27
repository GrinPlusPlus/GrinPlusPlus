#pragma once

#include <Core/Models/FullBlock.h>
#include <Core/Models/BlockSums.h>
#include <Database/BlockDb.h>
#include <memory>

// Forward Declarations
class BlindingFactor;
class ITxHashSet;

class BlockValidator
{
public:
	BlockValidator(const IBlockDB& blockDB, const ITxHashSet* pTxHashSet);

	std::unique_ptr<BlockSums> ValidateBlock(const FullBlock& block) const;
	bool IsBlockSelfConsistent(const FullBlock& block) const;

private:
	bool VerifyKernelLockHeights(const FullBlock& block) const;
	bool VerifyCoinbase(const FullBlock& block) const;

	const IBlockDB& m_blockDB;
	const ITxHashSet* m_pTxHashSet;
};