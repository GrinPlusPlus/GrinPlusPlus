#pragma once

#include <string>
#include <Core/Models/BlockSums.h>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/OutputLocation.h>
#include <Core/Models/DTOs/OutputRange.h>
#include <Core/Traits/Batchable.h>
#include <Crypto/Hash.h>

// Forward Declarations
class Config;
class FullBlock;
class Commitment;
class OutputIdentifier;
class IBlockChainServer;
class IBlockDB;
class Transaction;
class SyncStatus;

class ITxHashSet : public Traits::IBatchable
{
public:
	virtual ~ITxHashSet() = default;

	//
	// Validates all hashes, signatures, etc in the entire TxHashSet.
	// This is typically only used during initial sync.
	//
	virtual std::unique_ptr<BlockSums> ValidateTxHashSet(
		const BlockHeader& header,
		const IBlockChainServer& blockChainServer,
		SyncStatus& syncStatus
	) = 0;

	//
	// Saves the commitments, MMR indices, and block height for all unspent outputs in the block.
	// This is typically only used during initial sync.
	//
	virtual void SaveOutputPositions(
		std::shared_ptr<IBlockDB> pBlockDB,
		const BlockHeader& blockHeader,
		const uint64_t firstOutputIndex
	) = 0;



	//
	// Returns true if the output at the given location has not been spent.
	//
	virtual bool IsUnspent(
		const OutputLocation& location
	) const = 0;

	//
	// Returns true if all inputs in the transaction are valid and unspent. Otherwise, false.
	//
	virtual bool IsValid(
		std::shared_ptr<const IBlockDB> pBlockDB,
		const Transaction& transaction
	) const = 0;

	//
	// Appends all new kernels, outputs, and rangeproofs to the MMRs, and prunes all of the inputs.
	//
	virtual bool ApplyBlock(
		std::shared_ptr<IBlockDB> pBlockDB,
		const FullBlock& block
	) = 0;

	//
	// Validates that the kernel, output and rangeproof MMR roots match those specified in the given header.
	//
	virtual bool ValidateRoots(
		const BlockHeader& blockHeader
	) const = 0;



	//
	// Get last n kernel hashes.
	//
	virtual std::vector<Hash> GetLastKernelHashes(
		const uint64_t numberOfKernels
	) const = 0;

	//
	// Get last n output hashes.
	//
	virtual std::vector<Hash> GetLastOutputHashes(
		const uint64_t numberOfOutputs
	) const = 0;

	//
	// Get last n rangeproof hashes.
	//
	virtual std::vector<Hash> GetLastRangeProofHashes(
		const uint64_t numberOfRangeProofs
	) const = 0;

	//
	// Get outputs by leaf/insertion index.
	//
	virtual OutputRange GetOutputsByLeafIndex(
		std::shared_ptr<const IBlockDB> pBlockDB,
		const uint64_t startIndex,
		const uint64_t maxNumOutputs
	) const = 0;

	//
	// Get outputs by leaf/insertion index.
	//
	virtual std::vector<OutputDTO> GetOutputsByMMRIndex(
		std::shared_ptr<const IBlockDB> pBlockDB,
		const uint64_t startIndex,
		const uint64_t lastIndex
	) const = 0;

	virtual BlockHeaderPtr GetFlushedBlockHeader() const = 0;



	//
	// Rewinds the kernel, output, and rangeproof MMRs to the given block.
	//
	virtual bool Rewind(
		std::shared_ptr<const IBlockDB> pBlockDB,
		const BlockHeader& header
	) = 0;

	//
	// Flushes all changes to disk.
	//
	virtual void Commit() = 0;

	//
	// Discards all changes since the last commit.
	//
	virtual void Rollback() = 0;

	//
	// Removes pruned leaves and hashes from the output and rangeproof PMMRs to reduce disk usage.
	//
	virtual void Compact() = 0;
};

typedef std::shared_ptr<ITxHashSet> ITxHashSetPtr;
typedef std::shared_ptr<const ITxHashSet> ITxHashSetConstPtr;