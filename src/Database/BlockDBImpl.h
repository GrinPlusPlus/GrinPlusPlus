#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/transaction.h>

#include <Database/BlockDb.h>
#include <Config/Config.h>
#include <mutex>
#include <set>

using namespace rocksdb;

class BlockDB : public IBlockDB
{
public:
	~BlockDB();

	static std::shared_ptr<BlockDB> OpenDB(const Config& config);

	virtual void Commit() override final;
	virtual void Rollback() override final;
	virtual void OnInitWrite() override final;
	virtual void OnEndWrite() override final;

	Status Read(ColumnFamilyHandle* pFamilyHandle, const Slice& key, std::string* pValue) const;
	Status Write(ColumnFamilyHandle* pFamilyHandle, const Slice& key, const Slice& value);

	virtual BlockHeaderPtr GetBlockHeader(const Hash& hash) const override final;

	virtual void AddBlockHeader(BlockHeaderPtr pBlockHeader) override final;
	virtual void AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders) override final;

	virtual void AddBlock(const FullBlock& block) override final;
	virtual std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const override final;

	virtual void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) override final;
	virtual std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const override final;

	virtual void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) override final;
	virtual std::unique_ptr<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const override final;

	virtual void AddBlockInputBitmap(const Hash& blockHash, const Roaring& bitmap) override final;
	virtual std::unique_ptr<Roaring> GetBlockInputBitmap(const Hash& blockHash) const override final;

private:
	BlockDB(
		const Config& config,
		DB* pDatabase,
		OptimisticTransactionDB* pTransactionDB,
		ColumnFamilyHandle* pDefaultHandle,
		ColumnFamilyHandle* pBlockHandle,
		ColumnFamilyHandle* pHeaderHandle,
		ColumnFamilyHandle* pBlockSumsHandle,
		ColumnFamilyHandle* pOutputPosHandle,
		ColumnFamilyHandle* pInputBitmapHandle
	);

	const Config& m_config;

	DB* m_pDatabase;
	OptimisticTransactionDB* m_pTransactionDB;

	Transaction* m_pTransaction;

	ColumnFamilyHandle* m_pDefaultHandle;
	ColumnFamilyHandle* m_pBlockHandle;
	ColumnFamilyHandle* m_pHeaderHandle;
	ColumnFamilyHandle* m_pBlockSumsHandle;
	ColumnFamilyHandle* m_pOutputPosHandle;
	ColumnFamilyHandle* m_pInputBitmapHandle;

	std::vector<BlockHeaderPtr> m_uncommitted;
};