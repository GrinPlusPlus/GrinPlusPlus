#pragma once

#include "RocksDB/RocksDB.h"

#include <Database/BlockDb.h>
#include <Config/Config.h>
#include <caches/Cache.h>
#include <mutex>
#include <set>

class BlockDB : public IBlockDB
{
public:
	BlockDB(const Config& config, const std::shared_ptr<RocksDB>& pRocksDB)
		: m_config(config), m_pRocksDB(pRocksDB), m_blockHeadersCache(128) { }
	virtual ~BlockDB() = default;

	static std::shared_ptr<BlockDB> OpenDB(const Config& config);

	void Commit() final;
	void Rollback() noexcept final;
	void OnInitWrite() final { m_pRocksDB->OnInitWrite(); }
	void OnEndWrite() final { m_pRocksDB->OnEndWrite(); }

	BlockHeaderPtr GetBlockHeader(const Hash& hash) const final;

	void AddBlockHeader(BlockHeaderPtr pBlockHeader) final;
	void AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders) final;

	void AddBlock(const FullBlock& block) final;
	std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const final;

	void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) final;
	std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const final;
	void ClearBlockSums() final;

	void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) final;
	std::unique_ptr<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const final;
	void RemoveOutputPositions(const std::vector<Commitment>& outputCommitments) final;
	void ClearOutputPositions() final;

	void AddSpentPositions(const Hash& blockHash, const std::vector<SpentOutput>& outputPostions) final;
	std::unordered_map<Commitment, OutputLocation> GetSpentPositions(const Hash& blockHash) const final;
	void ClearSpentPositions() final;

private:
	//Status Read(ColumnFamilyHandle* pFamilyHandle, const Slice& key, std::string* pValue) const;
	//Status Write(ColumnFamilyHandle* pFamilyHandle, const Slice& key, const Slice& value);
	//Status Delete(ColumnFamilyHandle* pFamilyHandle, const Slice& key);
	//void DeleteAll(ColumnFamilyHandle* pFamilyHandle);

	const Config& m_config;
	std::shared_ptr<RocksDB> m_pRocksDB;
	FIFOCache<Hash, BlockHeaderPtr> m_blockHeadersCache;

	std::vector<BlockHeaderPtr> m_uncommitted;
};