#pragma once

#include <Roaring.h>

#include <Core/Models/BlockHeader.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/BlockSums.h>
#include <Core/Models/OutputLocation.h>
#include <Core/Models/SpentOutput.h>
#include <Core/Traits/Batchable.h>
#include <unordered_map>
#include <memory>

class IBlockDB : public Traits::IBatchable
{
public:
	virtual ~IBlockDB() = default;

	virtual BlockHeaderPtr GetBlockHeader(const Hash& hash) const = 0;

	virtual void AddBlockHeader(BlockHeaderPtr pBlockHeader) = 0;
	virtual void AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders) = 0;

	virtual void AddBlock(const FullBlock& block) = 0;
	virtual std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const = 0;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) = 0;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const = 0;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) = 0;
	virtual std::unique_ptr<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const = 0;
	virtual void RemoveOutputPositions(const std::vector<Commitment>& outputCommitments) = 0;

	virtual void AddBlockInputBitmap(const Hash& blockHash, const Roaring& bitmap) = 0;
	virtual std::unique_ptr<Roaring> GetBlockInputBitmap(const Hash& blockHash) const = 0;

	virtual void AddSpentPositions(const Hash& blockHash, const std::vector<SpentOutput>& outputPositions) = 0;
	virtual std::unordered_map<Commitment, OutputLocation> GetSpentPositions(const Hash& blockHash) const = 0;
};