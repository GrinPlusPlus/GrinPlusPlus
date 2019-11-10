#include "BlockDBImpl.h"

#include <Database/DatabaseException.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/StringUtil.h>
#include <utility>
#include <string>
#include <filesystem.h>

BlockDB::BlockDB(
	const Config& config,
	DB* pDatabase,
	OptimisticTransactionDB* pTransactionDB,
	ColumnFamilyHandle* pDefaultHandle,
	ColumnFamilyHandle* pBlockHandle,
	ColumnFamilyHandle* pHeaderHandle,
	ColumnFamilyHandle* pBlockSumsHandle,
	ColumnFamilyHandle* pOutputPosHandle,
	ColumnFamilyHandle* pInputBitmapHandle)
	: m_config(config),
	m_pDatabase(pDatabase),
	m_pTransactionDB(pTransactionDB),
	m_pTransaction(nullptr),
	m_pDefaultHandle(pDefaultHandle),
	m_pBlockHandle(pBlockHandle),
	m_pHeaderHandle(pHeaderHandle),
	m_pBlockSumsHandle(pBlockSumsHandle),
	m_pOutputPosHandle(pOutputPosHandle),
	m_pInputBitmapHandle(pInputBitmapHandle)
{

}

BlockDB::~BlockDB()
{
	delete m_pBlockHandle;
	delete m_pHeaderHandle;
	delete m_pBlockSumsHandle;
	delete m_pOutputPosHandle;
	delete m_pInputBitmapHandle;
	delete m_pDatabase;
}

std::shared_ptr<BlockDB> BlockDB::OpenDB(const Config& config)
{
	Options options;
	options.IncreaseParallelism();

	// create the DB if it's not already present
	options.create_if_missing = true;
	options.compression = kNoCompression;

	// open DB
	const std::string dbPath = config.GetDatabaseDirectory() + "CHAIN/";
	fs::create_directories(dbPath);

	ColumnFamilyDescriptor BLOCK_COLUMN = ColumnFamilyDescriptor("BLOCK", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor HEADER_COLUMN = ColumnFamilyDescriptor("HEADER", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor BLOCK_SUMS_COLUMN = ColumnFamilyDescriptor("BLOCK_SUMS", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor OUTPUT_POS_COLUMN = ColumnFamilyDescriptor("OUTPUT_POS", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor INPUT_BITMAP_COLUMN = ColumnFamilyDescriptor("INPUT_BITMAP", *ColumnFamilyOptions().OptimizeForPointLookup(1024));

	std::vector<std::string> columnFamilies;
	Status status = OptimisticTransactionDB::ListColumnFamilies(options, dbPath, &columnFamilies);

	OptimisticTransactionDB* pTransactionDB = nullptr;
	DB* pDatabase = nullptr;
	ColumnFamilyHandle* pDefaultHandle = nullptr;
	ColumnFamilyHandle* pBlockHandle = nullptr;
	ColumnFamilyHandle* pHeaderHandle = nullptr;
	ColumnFamilyHandle* pBlockSumsHandle = nullptr;
	ColumnFamilyHandle* pOutputPosHandle = nullptr;
	ColumnFamilyHandle* pInputBitmapHandle = nullptr;

	if (status.ok())
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor(), BLOCK_COLUMN, HEADER_COLUMN, BLOCK_SUMS_COLUMN, OUTPUT_POS_COLUMN, INPUT_BITMAP_COLUMN });
		std::vector<ColumnFamilyHandle*> columnHandles;

		status = OptimisticTransactionDB::Open(options, dbPath, columnDescriptors, &columnHandles, &pTransactionDB);
		if (!status.ok())
		{
			throw DATABASE_EXCEPTION("DB::Open failed with error: " + std::string(status.getState()));
		}

		pDatabase = pTransactionDB->GetBaseDB();

		pDefaultHandle = columnHandles[0];
		pBlockHandle = columnHandles[1];
		pHeaderHandle = columnHandles[2];
		pBlockSumsHandle = columnHandles[3];
		pOutputPosHandle = columnHandles[4];
		pInputBitmapHandle = columnHandles[5];
	}
	else
	{
		LOG_INFO("BlockDB not found. Creating it now.");

		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor() });
		std::vector<ColumnFamilyHandle*> columnHandles;
		status = OptimisticTransactionDB::Open(options, dbPath, columnDescriptors, &columnHandles, &pTransactionDB);
		if (!status.ok())
		{
			throw DATABASE_EXCEPTION("DB::Open failed with error: " + std::string(status.getState()));
		}

		pDatabase = pTransactionDB->GetBaseDB();

		pDefaultHandle = columnHandles[0];
		pDatabase->CreateColumnFamily(BLOCK_COLUMN.options, BLOCK_COLUMN.name, &pBlockHandle);
		pDatabase->CreateColumnFamily(HEADER_COLUMN.options, HEADER_COLUMN.name, &pHeaderHandle);
		pDatabase->CreateColumnFamily(BLOCK_SUMS_COLUMN.options, BLOCK_SUMS_COLUMN.name, &pBlockSumsHandle);
		pDatabase->CreateColumnFamily(OUTPUT_POS_COLUMN.options, OUTPUT_POS_COLUMN.name, &pOutputPosHandle);
		pDatabase->CreateColumnFamily(INPUT_BITMAP_COLUMN.options, INPUT_BITMAP_COLUMN.name, &pInputBitmapHandle);
	}

	return std::shared_ptr<BlockDB>(new BlockDB(
		config,
		pDatabase,
		pTransactionDB,
		pDefaultHandle,
		pBlockHandle,
		pHeaderHandle,
		pBlockSumsHandle,
		pOutputPosHandle,
		pInputBitmapHandle
	));
}

void BlockDB::Commit()
{
	const Status status = m_pTransaction->Commit();
	if (!status.ok())
	{
		LOG_ERROR_F("Transaction::Commit failed with error (%s)", status.getState());
		throw DATABASE_EXCEPTION("Transaction::Commit Failed with error: " + std::string(status.getState()));
	}
}

void BlockDB::Rollback()
{
	const Status status = m_pTransaction->Rollback();
	if (!status.ok())
	{
		LOG_ERROR_F("Transaction::Rollback failed with error (%s)", status.getState());
		throw DATABASE_EXCEPTION("Transaction::Rollback Failed with error: " + std::string(status.getState()));
	}
}

void BlockDB::OnInitWrite()
{
	m_pTransaction = m_pTransactionDB->BeginTransaction(WriteOptions());
}

void BlockDB::OnEndWrite()
{
	delete m_pTransaction;
	m_pTransaction = nullptr;
}

Status BlockDB::Read(ColumnFamilyHandle* pFamilyHandle, const Slice& key, std::string* pValue) const
{
	if (m_pTransaction != nullptr)
	{
		return m_pTransaction->Get(ReadOptions(), pFamilyHandle, key, pValue);
	}
	else
	{
		return m_pDatabase->Get(ReadOptions(), pFamilyHandle, key, pValue);
	}
}

Status BlockDB::Write(ColumnFamilyHandle* pFamilyHandle, const Slice& key, const Slice& value)
{
	if (m_pTransaction != nullptr)
	{
		return m_pTransaction->Put(pFamilyHandle, key, value);
	}
	else
	{
		return m_pDatabase->Put(WriteOptions(), pFamilyHandle, key, value);
	}
}

std::unique_ptr<BlockHeader> BlockDB::GetBlockHeader(const Hash& hash) const
{
	try
	{
		Slice key((const char*)hash.data(), hash.size());

		std::string value;
		const Status status = Read(m_pHeaderHandle, key, &value);
		if (status.ok())
		{
			std::vector<unsigned char> data(value.data(), value.data() + value.size());
			ByteBuffer byteBuffer(std::move(data));
			return std::make_unique<BlockHeader>(BlockHeader::Deserialize(byteBuffer));
		}
		else if (status.IsNotFound())
		{
			LOG_ERROR_F("Header not found for hash %s", hash);
			return nullptr;
		}
		else
		{
			LOG_ERROR_F("DB::Get failed for hash (%s) with error (%s)", hash, status.getState());
			throw DATABASE_EXCEPTION("DB::Get Failed with error: " + std::string(status.getState()));
		}
	}
	catch (DatabaseException&)
	{
		throw;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: %s", e.what());
		throw DATABASE_EXCEPTION(e.what());
	}
}

void BlockDB::AddBlockHeader(const BlockHeader& blockHeader)
{
	LOG_TRACE_F("Adding header %s", blockHeader);

	try
	{
		const Hash& hash = blockHeader.GetHash();

		Serializer serializer;
		blockHeader.Serialize(serializer);

		Slice key((const char*)hash.data(), hash.size());
		Slice value((const char*)serializer.data(), serializer.size());
		const Status status = Write(m_pHeaderHandle, Slice(key), value);
		if (!status.ok())
		{
			LOG_ERROR_F("DB::Put failed for header (%s) with error (%s)", hash, status.getState());
			throw DATABASE_EXCEPTION("DB::Put Failed with error: " + std::string(status.getState()));
		}
	}
	catch (DatabaseException&)
	{
		throw;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: %s", e.what());
		throw DATABASE_EXCEPTION("Error occurred: " + std::string(e.what()));
	}
}

void BlockDB::AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders)
{
	LOG_TRACE_F("Adding (%llu) headers.", blockHeaders.size());

	try
	{
		Status status;
		for (const BlockHeader& blockHeader : blockHeaders)
		{
			const Hash& hash = blockHeader.GetHash();

			Serializer serializer;
			blockHeader.Serialize(serializer);

			Slice key((const char*)hash.data(), hash.size());
			Slice value((const char*)serializer.data(), serializer.size());

			status = m_pTransaction->Put(m_pHeaderHandle, key, value);
			if (!status.ok())
			{
				LOG_ERROR_F("WriteBatch::put failed for header (%s) with error (%s)", blockHeader, status.getState());
				throw DATABASE_EXCEPTION("WriteBatch::put failed with error: " + std::string(status.getState()));
			}
		}
	}
	catch (DatabaseException&)
	{
		throw;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: %s", e.what());
		throw DATABASE_EXCEPTION("Error occurred: " + std::string(e.what()));
	}

	LOG_TRACE("Finished adding headers.");
}

void BlockDB::AddBlock(const FullBlock& block)
{
	LOG_TRACE("Adding block");
	const std::vector<unsigned char>& hash = block.GetHash().GetData();

	Serializer serializer;
	block.Serialize(serializer);

	Slice key((const char*)hash.data(), hash.size());
	Slice value((const char*)serializer.data(), serializer.size());
	const Status status = Write(m_pBlockHandle, Slice(key), value);
	if (!status.ok())
	{
		LOG_ERROR_F("Failed to save Block: %s", block);
		throw DATABASE_EXCEPTION("Failed to save Block.");
	}
}

std::unique_ptr<FullBlock> BlockDB::GetBlock(const Hash& hash) const
{
	std::unique_ptr<FullBlock> pBlock = std::unique_ptr<FullBlock>(nullptr);

	Slice key((const char*)hash.data(), hash.size());
	std::string value;
	Status s = Read(m_pBlockHandle, key, &value);
	if (s.ok())
	{
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(std::move(data));
		pBlock = std::make_unique<FullBlock>(FullBlock::Deserialize(byteBuffer));
	}

	return pBlock;
}

void BlockDB::AddBlockSums(const Hash& blockHash, const BlockSums& blockSums)
{
	LOG_TRACE_F("Adding BlockSums for block %s", blockHash);

	Slice key((const char*)blockHash.data(), blockHash.size());

	// Serializes the BlockSums object
	Serializer serializer;
	blockSums.Serialize(serializer);
	Slice value((const char*)serializer.data(), serializer.size());

	// Insert BlockSums object
	const Status status = Write(m_pBlockSumsHandle, key, value);
	if (!status.ok())
	{
		LOG_ERROR_F("Failed to save BlockSums for %s", blockHash);
		throw DATABASE_EXCEPTION("Failed to save BlockSums.");
	}
}

std::unique_ptr<BlockSums> BlockDB::GetBlockSums(const Hash& blockHash) const
{
	std::unique_ptr<BlockSums> pBlockSums = std::unique_ptr<BlockSums>(nullptr);

	// Read from DB
	Slice key((const char*)blockHash.data(), blockHash.size());
	std::string value;
	const Status s = Read(m_pBlockSumsHandle, key, &value);
	if (s.ok())
	{
		// Deserialize result
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(std::move(data));
		pBlockSums = std::make_unique<BlockSums>(BlockSums::Deserialize(byteBuffer));
	}

	return pBlockSums;
}

void BlockDB::AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location)
{
	Slice key((const char*)outputCommitment.GetBytes().data(), 32);

	// Serializes the output position
	Serializer serializer;
	location.Serialize(serializer);
	Slice value((const char*)serializer.data(), serializer.size());

	// Insert the output position
	const Status status = Write(m_pOutputPosHandle, key, value);
	if (!status.ok())
	{
		LOG_ERROR_F("Failed to save location for output %s", outputCommitment);
		throw DATABASE_EXCEPTION("Failed to save output location.");
	}
}

std::unique_ptr<OutputLocation> BlockDB::GetOutputPosition(const Commitment& outputCommitment) const
{
	std::unique_ptr<OutputLocation> pOutputPosition = nullptr;

	Slice key((const char*)outputCommitment.GetBytes().data(), 32);

	// Read from DB
	std::string value;
	const Status s = Read(m_pOutputPosHandle, key, &value);
	if (s.ok())
	{
		// Deserialize result
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(std::move(data));
		pOutputPosition = std::make_unique<OutputLocation>(OutputLocation::Deserialize(byteBuffer));
	}

	return pOutputPosition;
}

void BlockDB::AddBlockInputBitmap(const Hash& blockHash, const Roaring& bitmap)
{
	try
	{
		Slice key((const char*)blockHash.data(), blockHash.size());

		// Serializes the bitmap
		const size_t bitmapSize = bitmap.getSizeInBytes();
		std::vector<char> serializedBitmap(bitmapSize);
		const size_t bytesWritten = bitmap.write(serializedBitmap.data(), true);
		if (bytesWritten != bitmapSize)
		{
			LOG_ERROR_F("Expected to write %llu bytes but wrote %llu", bitmapSize, bytesWritten);
			throw DATABASE_EXCEPTION("Roaring bitmap did not serialize to expected number of bytes.");
		}

		Slice value(serializedBitmap.data(), bitmapSize);

		// Insert the output position
		const Status status = Write(m_pInputBitmapHandle, key, value);
	}
	catch (DatabaseException&)
	{
		throw;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: %s", e.what());
		throw DATABASE_EXCEPTION(e.what());
	}
}

std::unique_ptr<Roaring> BlockDB::GetBlockInputBitmap(const Hash& blockHash) const
{
	try
	{
		Slice key((const char*)blockHash.data(), blockHash.size());

		// Read from DB
		std::string value;
		const Status s = Read(m_pInputBitmapHandle, key, &value);
		if (s.ok())
		{
			// Deserialize result
			return std::make_unique<Roaring>(Roaring::readSafe(value.data(), value.size()));
		}
		else if (s.IsNotFound())
		{
			LOG_ERROR_F("Block input bitmap not found for block %s", blockHash);
			return nullptr;
		}
		else
		{
			LOG_ERROR_F("DB::Get failed for block (%s) with error (%s)", blockHash, s.getState());
			throw DATABASE_EXCEPTION("DB::Get Failed with error: " + std::string(s.getState()));
		}
	}
	catch (DatabaseException&)
	{
		throw;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: %s", e.what());
		throw DATABASE_EXCEPTION(e.what());
	}
}