#pragma once

#include <string>
#include <Core/Models/BlockSums.h>
#include <Core/Models/OutputLocation.h>
#include <PMMR/OutputRange.h>
#include <Crypto/Hash.h>

// Forward Declarations
class Config;
class FullBlock;
class BlockHeader;
class Commitment;
class OutputIdentifier;
class IBlockChainServer;
class IBlockDB;
class Transaction;
class SyncStatus;

class ITxHashSet
{
public:
	//
	// Validates all hashes, signatures, etc in the entire TxHashSet.
	// This is typically only used during initial sync.
	//
	virtual std::unique_ptr<BlockSums> ValidateTxHashSet(const BlockHeader& header, const IBlockChainServer& blockChainServer, SyncStatus& syncStatus) = 0;

	//
	// Saves the commitments, MMR indices, and block height for all unspent outputs in the block.
	// This is typically only used during initial sync.
	//
	virtual bool SaveOutputPositions(const BlockHeader& blockHeader, const uint64_t firstOutputIndex) = 0;



	//
	// Returns true if the output at the given location has not been spent.
	//
	virtual bool IsUnspent(const OutputLocation& location) const = 0;

	//
	// Returns true if all inputs in the transaction are valid and unspent. Otherwise, false.
	//
	virtual bool IsValid(const Transaction& transaction) const = 0;

	//
	// Appends all new kernels, outputs, and rangeproofs to the MMRs, and prunes all of the inputs.
	//
	virtual bool ApplyBlock(const FullBlock& block) = 0;

	//
	// Validates that the kernel, output and rangeproof MMR roots match those specified in the given header.
	//
	virtual bool ValidateRoots(const BlockHeader& blockHeader) const = 0;



	//
	// Get last n kernel hashes.
	//
	virtual std::vector<Hash> GetLastKernelHashes(const uint64_t numberOfKernels) const = 0;

	//
	// Get last n output hashes.
	//
	virtual std::vector<Hash> GetLastOutputHashes(const uint64_t numberOfOutputs) const = 0;

	//
	// Get last n rangeproof hashes.
	//
	virtual std::vector<Hash> GetLastRangeProofHashes(const uint64_t numberOfRangeProofs) const = 0;

	//
	// Get outputs by leaf/insertion index.
	//
	virtual OutputRange GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const = 0;

	//
	// Get outputs by leaf/insertion index.
	//
	virtual std::vector<OutputDisplayInfo> GetOutputsByMMRIndex(const uint64_t startIndex, const uint64_t lastIndex) const = 0;



	//
	// Rewinds the kernel, output, and rangeproof MMRs to the given block.
	//
	virtual bool Rewind(const BlockHeader& header) = 0;

	//
	// Flushes all changes to disk.
	//
	virtual bool Commit() = 0;

	//
	// Discards all changes since the last commit.
	//
	virtual bool Discard() = 0;

	
	virtual bool Compact() = 0;
};