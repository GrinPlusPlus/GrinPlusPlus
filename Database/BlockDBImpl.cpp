#include "BlockDBImpl.h"

#include <Infrastructure/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <utility>
#include <string>
#include <filesystem.h>

BlockDB::BlockDB(const Config& config)
	: m_config(config)
{

}

void BlockDB::OpenDB()
{
	Options options;
	// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
	options.IncreaseParallelism();
	//options.max_open_files = 25;

	//options.OptimizeLevelStyleCompaction();
	// create the DB if it's not already present
	options.create_if_missing = true;
	options.compression = kNoCompression;

	// open DB
	const std::string dbPath = m_config.GetDatabaseDirectory() + "CHAIN/";
	fs::create_directories(dbPath);

	ColumnFamilyDescriptor BLOCK_COLUMN = ColumnFamilyDescriptor("BLOCK", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor HEADER_COLUMN = ColumnFamilyDescriptor("HEADER", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor BLOCK_SUMS_COLUMN = ColumnFamilyDescriptor("BLOCK_SUMS", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor OUTPUT_POS_COLUMN = ColumnFamilyDescriptor("OUTPUT_POS", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
	ColumnFamilyDescriptor INPUT_BITMAP_COLUMN = ColumnFamilyDescriptor("INPUT_BITMAP", *ColumnFamilyOptions().OptimizeForPointLookup(1024));

	std::vector<std::string> columnFamilies;
	DB::ListColumnFamilies(options, dbPath, &columnFamilies);

	if (columnFamilies.size() <= 1)
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor() });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);

		m_pDefaultHandle = columnHandles[0];
		m_pDatabase->CreateColumnFamily(BLOCK_COLUMN.options, BLOCK_COLUMN.name, &m_pBlockHandle);
		m_pDatabase->CreateColumnFamily(HEADER_COLUMN.options, HEADER_COLUMN.name, &m_pHeaderHandle);
		m_pDatabase->CreateColumnFamily(BLOCK_SUMS_COLUMN.options, BLOCK_SUMS_COLUMN.name, &m_pBlockSumsHandle);
		m_pDatabase->CreateColumnFamily(OUTPUT_POS_COLUMN.options, OUTPUT_POS_COLUMN.name, &m_pOutputPosHandle);
		m_pDatabase->CreateColumnFamily(INPUT_BITMAP_COLUMN.options, INPUT_BITMAP_COLUMN.name, &m_pInputBitmapHandle);
	}
	else if (columnFamilies.size() == 5)
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor(), BLOCK_COLUMN, HEADER_COLUMN, BLOCK_SUMS_COLUMN, OUTPUT_POS_COLUMN });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);
		m_pDefaultHandle = columnHandles[0];
		m_pBlockHandle = columnHandles[1];
		m_pHeaderHandle = columnHandles[2];
		m_pBlockSumsHandle = columnHandles[3];
		m_pOutputPosHandle = columnHandles[4];
		m_pDatabase->CreateColumnFamily(INPUT_BITMAP_COLUMN.options, INPUT_BITMAP_COLUMN.name, &m_pInputBitmapHandle);
	}
	else
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor(), BLOCK_COLUMN, HEADER_COLUMN, BLOCK_SUMS_COLUMN, OUTPUT_POS_COLUMN, INPUT_BITMAP_COLUMN });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);
		m_pDefaultHandle = columnHandles[0];
		m_pBlockHandle = columnHandles[1];
		m_pHeaderHandle = columnHandles[2];
		m_pBlockSumsHandle = columnHandles[3];
		m_pOutputPosHandle = columnHandles[4];
		m_pInputBitmapHandle = columnHandles[5];
	}
}

void BlockDB::CloseDB()
{
	

	//delete m_pDefaultHandle;
	delete m_pBlockHandle;
	delete m_pHeaderHandle;
	delete m_pBlockSumsHandle;
	delete m_pOutputPosHandle;
	delete m_pInputBitmapHandle;
	delete m_pDatabase;
}

std::vector<BlockHeader*> BlockDB::LoadBlockHeaders(const std::vector<Hash>& hashes) const
{
	LoggerAPI::LogTrace("BlockDB::LoadBlockHeaders - Loading headers - " + std::to_string(hashes.size()));

	std::vector<BlockHeader*> blockHeaders;
	blockHeaders.reserve(hashes.size());

	for (const Hash& hash : hashes)
	{
		const Slice key((const char*)&hash[0], hash.GetData().size());
		std::string value;
		Status s = m_pDatabase->Get(ReadOptions(), m_pHeaderHandle, key, &value);
		if (s.ok())
		{
			std::vector<unsigned char> data(value.data(), value.data() + value.size());
			ByteBuffer byteBuffer(data);
			BlockHeader* pBlockHeader = new BlockHeader(BlockHeader::Deserialize(byteBuffer));
			blockHeaders.push_back(pBlockHeader);
		}
	}

	LoggerAPI::LogTrace("BlockDB::LoadBlockHeaders - Finished loading headers.");
	return blockHeaders;
}

std::unique_ptr<BlockHeader> BlockDB::GetBlockHeader(const Hash& hash) const
{
	std::unique_ptr<BlockHeader> pHeader = std::unique_ptr<BlockHeader>(nullptr);

	Slice key((const char*)&hash[0], 32);
	std::string value;
	Status s = m_pDatabase->Get(ReadOptions(), m_pHeaderHandle, key, &value);
	if (s.ok())
	{
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(data);
		pHeader = std::make_unique<BlockHeader>(BlockHeader::Deserialize(byteBuffer));
	}

	return pHeader;
}

void BlockDB::AddBlockHeader(const BlockHeader& blockHeader)
{
	LoggerAPI::LogTrace("BlockDB::AddBlockHeader - Adding header");
	const std::vector<unsigned char>& hash = blockHeader.GetHash().GetData();

	Serializer serializer;
	blockHeader.Serialize(serializer);

	Slice key((const char*)&hash[0], hash.size());
	Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());
	m_pDatabase->Put(WriteOptions(), m_pHeaderHandle, Slice(key), value);
}

void BlockDB::AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders)
{
	LoggerAPI::LogTrace("BlockDB::AddBlockHeaders - Adding headers - " + std::to_string(blockHeaders.size()));

	for (const BlockHeader& blockHeader : blockHeaders)
	{
		const std::vector<unsigned char>& hash = blockHeader.GetHash().GetData();

		Serializer serializer;
		blockHeader.Serialize(serializer);

		Slice key((const char*)&hash[0], hash.size());
		Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());
		m_pDatabase->Put(WriteOptions(), m_pHeaderHandle, key, value);
	}

	LoggerAPI::LogTrace("BlockDB::AddBlockHeaders - Finished adding headers.");
}

void BlockDB::AddBlock(const FullBlock& block)
{
	LoggerAPI::LogTrace("BlockDB::AddBlock - Adding block");
	const std::vector<unsigned char>& hash = block.GetHash().GetData();

	Serializer serializer;
	block.Serialize(serializer);

	Slice key((const char*)&hash[0], hash.size());
	Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());
	m_pDatabase->Put(WriteOptions(), m_pBlockHandle, Slice(key), value);
}

std::unique_ptr<FullBlock> BlockDB::GetBlock(const Hash& hash) const
{
	std::unique_ptr<FullBlock> pBlock = std::unique_ptr<FullBlock>(nullptr);

	Slice key((const char*)&hash[0], 32);
	std::string value;
	Status s = m_pDatabase->Get(ReadOptions(), m_pBlockHandle, key, &value);
	if (s.ok())
	{
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(data);
		pBlock = std::make_unique<FullBlock>(FullBlock::Deserialize(byteBuffer));
	}

	return pBlock;
}

void BlockDB::AddBlockSums(const Hash& blockHash, const BlockSums& blockSums)
{
	LoggerAPI::LogTrace("BlockDB::AddBlockSums - Adding BlockSums for block " + HexUtil::ConvertHash(blockHash));

	Slice key((const char*)&blockHash[0], 32);

	// Serializes the BlockSums object
	Serializer serializer;
	blockSums.Serialize(serializer);
	Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());

	// Insert BlockSums object
	m_pDatabase->Put(WriteOptions(), m_pBlockSumsHandle, key, value);
}

std::unique_ptr<BlockSums> BlockDB::GetBlockSums(const Hash& blockHash) const
{
	std::unique_ptr<BlockSums> pBlockSums = std::unique_ptr<BlockSums>(nullptr);

	// Read from DB
	Slice key((const char*)&blockHash[0], 32);
	std::string value;
	const Status s = m_pDatabase->Get(ReadOptions(), m_pBlockSumsHandle, key, &value);
	if (s.ok())
	{
		// Deserialize result
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(data);
		pBlockSums = std::make_unique<BlockSums>(BlockSums::Deserialize(byteBuffer));
	}

	return pBlockSums;
}

void BlockDB::AddOutputPosition(const Commitment& outputCommitment, const OutputLocation& location)
{
	const std::string outputHex = outputCommitment.ToHex();
	//LoggerAPI::LogTrace(StringUtil::Format("BlockDB::AddOutputPosition - Adding position (%llu) at height (%llu) for output (%s).", location.GetMMRIndex(), location.GetBlockHeight(), outputHex.c_str()));

	Slice key((const char*)&outputCommitment.GetCommitmentBytes()[0], 32);

	// Serializes the output position
	Serializer serializer;
	location.Serialize(serializer);
	Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());

	// Insert the output position
	m_pDatabase->Put(WriteOptions(), m_pOutputPosHandle, key, value);
}

std::optional<OutputLocation> BlockDB::GetOutputPosition(const Commitment& outputCommitment) const
{
	std::optional<OutputLocation> outputPosition = std::nullopt;

	Slice key((const char*)&outputCommitment.GetCommitmentBytes()[0], 32);

	// Read from DB
	std::string value;
	const Status s = m_pDatabase->Get(ReadOptions(), m_pOutputPosHandle, key, &value);
	if (s.ok())
	{
		// Deserialize result
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(data);
		outputPosition = std::make_optional<OutputLocation>(OutputLocation::Deserialize(byteBuffer));
	}

	return outputPosition;
}

void BlockDB::AddBlockInputBitmap(const Hash& blockHash, const Roaring& bitmap)
{
	Slice key((const char*)&blockHash[0], 32);

	// Serializes the bitmap
	const size_t bitmapSize = bitmap.getSizeInBytes();
	std::vector<char> serializedBitmap(bitmapSize);
	bitmap.write(serializedBitmap.data(), true);

	Slice value(serializedBitmap.data(), bitmapSize);

	// Insert the output position
	m_pDatabase->Put(WriteOptions(), m_pInputBitmapHandle, key, value);
}

std::optional<Roaring> BlockDB::GetBlockInputBitmap(const Hash& blockHash) const
{
	std::optional<Roaring> blockInputBitmap = std::nullopt;

	Slice key((const char*)&blockHash[0], 32);

	// Read from DB
	std::string value;
	const Status s = m_pDatabase->Get(ReadOptions(), m_pInputBitmapHandle, key, &value);
	if (s.ok())
	{
		// Deserialize result
		blockInputBitmap = std::make_optional<Roaring>(Roaring::readSafe(value.data(), value.size()));
	}

	return blockInputBitmap;
}