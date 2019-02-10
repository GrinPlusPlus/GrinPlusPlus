#pragma once

#include <Core/BlockHeader.h>
#include <Core/FullBlock.h>
#include <Core/BlockSums.h>
#include <Core/ChainType.h>
#include <Core/OutputLocation.h>
#include <memory>
#include <optional>

class IBlockDB
{
public:
	virtual std::vector<BlockHeader*> LoadBlockHeaders(const std::vector<Hash>& hashes) const = 0;
	virtual std::unique_ptr<BlockHeader> GetBlockHeader(const Hash& hash) const = 0;

	virtual void AddBlockHeader(const BlockHeader& blockHeader) = 0;
	virtual void AddBlockHeaders(const std::vector<BlockHeader*>& blockHeaders) = 0;

	virtual void AddBlock(const FullBlock& block) = 0;
	virtual std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const = 0;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) = 0;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const = 0;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) = 0;
	virtual std::optional<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const = 0;
};