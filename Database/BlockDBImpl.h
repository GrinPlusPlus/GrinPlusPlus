#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

#include <Database/BlockDb.h>
#include <Config/Config.h>
#include <Core/BlockHeader.h>
#include <mutex>
#include <set>

using namespace rocksdb;

class BlockDB : public IBlockDB
{
public:
	BlockDB(const Config& config);
	~BlockDB();

	void OpenDB();
	void CloseDB();

	virtual std::vector<BlockHeader*> LoadBlockHeaders(const std::vector<Hash>& hashes) override final;
	virtual std::unique_ptr<BlockHeader> GetBlockHeader(const Hash& hash) override final;

	virtual void AddBlockHeader(const BlockHeader& blockHeader) override final;
	virtual void AddBlockHeaders(const std::vector<BlockHeader*>& blockHeaders) override final;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) override final;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) override final;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const uint64_t mmrIndex) override final;
	virtual std::optional<uint64_t> GetOutputPosition(const Commitment& outputCommitment) override final;

private:
	std::string GetHeadKey(const EChainType chainType) const;

	const Config& m_config;

	DB* m_pDatabase;
	std::mutex m_mutex;
};