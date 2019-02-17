#pragma once

#include <Core/Models/BlockHeader.h>
#include <Core/Models/BlockSums.h>

// Forward Declarations
class HashFile;
class TxHashSet;
class KernelMMR;
class IBlockChainServer;
class MMR;
class Commitment;

class TxHashSetValidator
{
public:
	TxHashSetValidator(const IBlockChainServer& blockChainServer);

	std::unique_ptr<BlockSums> Validate(TxHashSet& txHashSet, const BlockHeader& blockHeader) const;

private:
	bool ValidateSizes(TxHashSet& txHashSet, const BlockHeader& blockHeader) const;
	bool ValidateMMRHashes(const MMR& mmr) const;

	bool ValidateKernelHistory(const KernelMMR& kernelMMR, const BlockHeader& blockHeader) const;
	bool ValidateRangeProofs(TxHashSet& txHashSet, const BlockHeader& blockHeader) const;

	const IBlockChainServer& m_blockChainServer;
};