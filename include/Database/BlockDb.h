#pragma once

#include <Core/Models/BlockHeader.h>
#include <Core/Models/SpentOutput.h>
#include <Core/Traits/Batchable.h>
#include <unordered_map>
#include <memory>

// Forward Declarations
class BlockSums;
class FullBlock;
class Commitment;
class OutputLocation;

class IBlockDB : public Traits::IBatchable
{
public:
	virtual ~IBlockDB() = default;

	/// <summary>
	/// Looks up the current serialization format version of the DB.
	/// </summary>
	/// <returns>The current DB format version.</returns>
	virtual uint8_t GetVersion() const = 0;

	/// <summary>
	/// Updates the DB serialization format version.
	/// </summary>
	/// <param name="version">The new version.</param>
	virtual void SetVersion(const uint8_t version) = 0;

	/// <summary>
	/// Updates FullBlock entries to the latest serialization format.
	/// </summary>
	virtual void MigrateBlocks() = 0;

	virtual BlockHeaderPtr GetBlockHeader(const Hash& hash) const = 0;

	virtual void AddBlockHeader(BlockHeaderPtr pBlockHeader) = 0;
	virtual void AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders) = 0;

	virtual void AddBlock(const FullBlock& block) = 0;
	virtual std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const = 0;
	virtual void ClearBlocks() = 0;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) = 0;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const = 0;
	virtual void ClearBlockSums() = 0;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) = 0;
	virtual std::unique_ptr<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const = 0;
	virtual void RemoveOutputPositions(const std::vector<Commitment>& outputCommitments) = 0;
	virtual void ClearOutputPositions() = 0;

	virtual void AddSpentPositions(const Hash& blockHash, const std::vector<SpentOutput>& outputPositions) = 0;
	virtual std::unordered_map<Commitment, OutputLocation> GetSpentPositions(const Hash& blockHash) const = 0;
	virtual void ClearSpentPositions() = 0;
};