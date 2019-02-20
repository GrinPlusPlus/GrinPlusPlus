#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

#include <Database/BlockDb.h>
#include <Config/Config.h>
#include <mutex>
#include <set>

using namespace rocksdb;

class BlockDB : public IBlockDB
{
public:
	BlockDB(const Config& config);
	~BlockDB() = default;

	void OpenDB();
	void CloseDB();

	virtual std::vector<BlockHeader*> LoadBlockHeaders(const std::vector<Hash>& hashes) const override final;
	virtual std::unique_ptr<BlockHeader> GetBlockHeader(const Hash& hash) const override final;

	virtual void AddBlockHeader(const BlockHeader& blockHeader) override final;
	virtual void AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders) override final;

	virtual void AddBlock(const FullBlock& block) override final;
	virtual std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const override final;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) override final;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const override final;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) override final;
	virtual std::optional<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const override final;

	virtual void AddBlockInputBitmap(const Hash& blockHash, const Roaring& bitmap) override final;
	virtual std::optional<Roaring> GetBlockInputBitmap(const Hash& blockHash) const override final;

private:
	const Config& m_config;

	DB* m_pDatabase;

	ColumnFamilyHandle* m_pDefaultHandle;
	ColumnFamilyHandle* m_pBlockHandle;
	ColumnFamilyHandle* m_pHeaderHandle;
	ColumnFamilyHandle* m_pBlockSumsHandle;
	ColumnFamilyHandle* m_pOutputPosHandle;
	ColumnFamilyHandle* m_pInputBitmapHandle;
};