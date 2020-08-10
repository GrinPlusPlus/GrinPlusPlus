#pragma once

#include <Core/Models/BlockHeader.h>
#include <Core/Models/BlockSums.h>
#include <P2P/SyncStatus.h>
#include "Common/HashFile.h"

// Forward Declarations
class TxHashSet;
class KernelMMR;
class IBlockChain;
class MMR;
class Commitment;

class TxHashSetValidator
{
public:
	TxHashSetValidator(const IBlockChain& blockChain)
		: m_blockChain(blockChain) { }

	std::unique_ptr<BlockSums> Validate(
		TxHashSet& txHashSet,
		const BlockHeader& blockHeader,
		SyncStatus& syncStatus
	) const;

private:
	bool ValidateSizes(TxHashSet& txHashSet, const BlockHeader& blockHeader) const;
	bool ValidateMMRHashes(std::shared_ptr<const MMR> pMMR) const;

	bool ValidateKernelHistory(
		const KernelMMR& kernelMMR,
		const BlockHeader& blockHeader,
		SyncStatus& syncStatus
	) const;

	BlockSums ValidateKernelSums(
		TxHashSet& txHashSet,
		const BlockHeader& blockHeader
	) const;

	bool ValidateRangeProofs(
		TxHashSet& txHashSet,
		SyncStatus& syncStatus
	) const;

	bool ValidateKernelSignatures(
		const KernelMMR& kernelMMR,
		SyncStatus& syncStatus
	) const;

	const IBlockChain& m_blockChain;
};