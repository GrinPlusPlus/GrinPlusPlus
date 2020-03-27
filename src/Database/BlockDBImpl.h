#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/transaction.h>

#include <Database/BlockDb.h>
#include <Config/Config.h>
#include <caches/Cache.h>
#include <mutex>
#include <set>

using namespace rocksdb;

class BlockDB : public IBlockDB
{
public:
	~BlockDB();

	static std::shared_ptr<BlockDB> OpenDB(const Config& config);

	void Commit() final;
	void Rollback() noexcept final;
	void OnInitWrite() final;
	void OnEndWrite() final;

	Status Read(ColumnFamilyHandle* pFamilyHandle, const Slice& key, std::string* pValue) const;
	Status Write(ColumnFamilyHandle* pFamilyHandle, const Slice& key, const Slice& value);
	Status Delete(ColumnFamilyHandle* pFamilyHandle, const Slice& key);

	BlockHeaderPtr GetBlockHeader(const Hash& hash) const final;

	void AddBlockHeader(BlockHeaderPtr pBlockHeader) final;
	void AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders) final;

	void AddBlock(const FullBlock& block) final;
	std::unique_ptr<FullBlock> GetBlock(const Hash& hash) const final;

	void AddBlockSums(const Hash& blockHash, const BlockSums& blockSums) final;
	std::unique_ptr<BlockSums> GetBlockSums(const Hash& blockHash) const final;

	void AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location) final;
	std::unique_ptr<OutputLocation> GetOutputPosition(const Commitment& outputCommitment) const final;
	void RemoveOutputPositions(const std::vector<Commitment>& outputCommitments) final;

	void AddBlockInputBitmap(const Hash& blockHash, const Roaring& bitmap) final;
	std::unique_ptr<Roaring> GetBlockInputBitmap(const Hash& blockHash) const final;

	void AddSpentPositions(const Hash& blockHash, const std::vector<SpentOutput>& outputPostions) final;
	std::unordered_map<Commitment, OutputLocation> GetSpentPositions(const Hash& blockHash) const final;

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
		ColumnFamilyHandle* pInputBitmapHandle,
		ColumnFamilyHandle* pSpentOutputsHandle
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
	ColumnFamilyHandle* m_pSpentOutputsHandle;

	std::vector<BlockHeaderPtr> m_uncommitted;

	FIFOCache<Hash, BlockHeaderPtr> m_blockHeadersCache;
};