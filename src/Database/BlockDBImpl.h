#pragma once

#include <Database/BlockDb.h>
#include <Core/Config.h>
#include <caches/Cache.h>
#include <mutex>
#include <set>

// Forward Declarations
class RocksDB;

class BlockDB : public IBlockDB
{
public:
	BlockDB(const Config& config, const std::shared_ptr<RocksDB>& pRocksDB)
		: m_config(config), m_pRocksDB(pRocksDB), m_blockHeadersCache(128) { }
	virtual ~BlockDB() = default;

	static std::shared_ptr<BlockDB> OpenDB(const Config& config);

	void Commit() final;
	void Rollback() noexcept final;
	void OnInitWrite(const bool batch) final;
	void OnEndWrite() final;

	uint8_t GetVersion() const final;
	void SetVersion(const uint8_t version) final;

	void MigrateBlocks() final;
	void Compact(const std::shared_ptr<const Chain>& pChain) final;

	BlockHeaderPtr GetBlockHeader(const Hash& hash) const final;

	void AddBlockHeader(BlockHeaderPtr pBlockHeader) final;
	void AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders) final;

	void AddBlock(const FullBlock& block) final;
	std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const final;
	void ClearBlocks() final;

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
	const Config& m_config;
	std::shared_ptr<RocksDB> m_pRocksDB;
	FIFOCache<Hash, BlockHeaderPtr> m_blockHeadersCache;

	std::vector<BlockHeaderPtr> m_uncommitted;
};