#pragma once

#include <Core/BlockHeader.h>
#include <Core/BlockSums.h>
#include <Core/ChainType.h>
#include <memory>
#include <optional>

// TODO: Handle DB Transactions (commits and rollbacks)
class IBlockDB
{
public:
	virtual std::vector<BlockHeader*> LoadBlockHeaders(const std::vector<Hash>& hashes) = 0;
	virtual std::unique_ptr<BlockHeader> GetBlockHeader(const Hash& hash) = 0;

	virtual void AddBlockHeader(const BlockHeader& blockHeader) = 0;
	virtual void AddBlockHeaders(const std::vector<BlockHeader*>& blockHeaders) = 0;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) = 0;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) = 0;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const uint64_t mmrIndex) = 0;
	virtual std::optional<uint64_t> GetOutputPosition(const Commitment& outputCommitment) = 0;
};