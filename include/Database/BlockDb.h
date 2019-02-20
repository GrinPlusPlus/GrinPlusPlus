#pragma once

#pragma warning(push)
#pragma warning(disable:4018)
#pragma warning(disable:4127)
#pragma warning(disable:4244)
#include <Core/CRoaring/roaring.hh>
#pragma warning(pop)

#include <Core/Models/BlockHeader.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/BlockSums.h>
#include <BlockChain/ChainType.h>
#include <Core/Models/OutputLocation.h>
#include <memory>
#include <optional>

class IBlockDB
{
public:
	virtual std::vector<BlockHeader*> LoadBlockHeaders(const std::vector<Hash>& hashes) const = 0;
	virtual std::unique_ptr<BlockHeader> GetBlockHeader(const Hash& hash) const = 0;

	virtual void AddBlockHeader(const BlockHeader& blockHeader) = 0;
	virtual void AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders) = 0;

	virtual void AddBlock(const FullBlock& block) = 0;
	virtual std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const = 0;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) = 0;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const = 0;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) = 0;
	virtual std::optional<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const = 0;

	virtual void AddBlockInputBitmap(const Hash& blockHash, const Roaring& bitmap) = 0;
	virtual std::optional<Roaring> GetBlockInputBitmap(const Hash& blockHash) const = 0;
};