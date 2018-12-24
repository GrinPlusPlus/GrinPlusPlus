#include "BlockDBImpl.h"

#include <Infrastructure/Logger.h>
#include <HexUtil.h>
#include <StringUtil.h>
#include <utility>
#include <string>
#include <filesystem>

const std::string BLOCK_SUMS_KEY = "SUMS_";
const std::string OUTPUT_POS_KEY = "OUT_";

std::string kDBPath = "/tmp/rocksdb_simple_example";

BlockDB::BlockDB(const Config& config)
	: m_config(config)
{

}

BlockDB::~BlockDB()
{
	CloseDB();
}

void BlockDB::OpenDB()
{
	Options options;
	// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
	options.IncreaseParallelism();
	//options.OptimizeLevelStyleCompaction();
	// create the DB if it's not already present
	options.create_if_missing = true;
	options.compression = kNoCompression;

	// open DB
	const std::string dbPath = m_config.GetDatabaseDirectory() + "BLOCKS/";
	std::filesystem::create_directories(dbPath);
	Status s = DB::Open(options, dbPath, &m_pDatabase);
}

void BlockDB::CloseDB()
{
	m_pDatabase->Close();
	//delete m_pDatabase;
}

std::vector<BlockHeader*> BlockDB::LoadBlockHeaders(const std::vector<Hash>& hashes)
{
	LoggerAPI::LogInfo("BlockDB::LoadBlockHeaders - Loading headers - " + std::to_string(hashes.size()));
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	std::vector<BlockHeader*> blockHeaders;
	blockHeaders.reserve(hashes.size());

	for (const Hash& hash : hashes)
	{
		const Slice key((const char*)&hash[0], hash.GetData().size());
		std::string value;
		Status s = m_pDatabase->Get(ReadOptions(), key, &value);
		if (s.ok())
		{
			std::vector<unsigned char> data(value.data(), value.data() + value.size());
			ByteBuffer byteBuffer(data);
			BlockHeader* pBlockHeader = new BlockHeader(BlockHeader::Deserialize(byteBuffer));
			blockHeaders.push_back(pBlockHeader);
		}
	}

	LoggerAPI::LogInfo("BlockDB::LoadBlockHeaders - Finished loading headers.");
	return blockHeaders;
}

std::unique_ptr<BlockHeader> BlockDB::GetBlockHeader(const Hash& hash)
{
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	std::unique_ptr<BlockHeader> pHeader = std::unique_ptr<BlockHeader>(nullptr);

	Slice key((const char*)&hash[0], 32);
	std::string value;
	Status s = m_pDatabase->Get(ReadOptions(), HexUtil::ConvertToHex(hash.GetData(), false, false), &value);
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
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	const std::vector<unsigned char>& hash = blockHeader.GetHash().GetData();

	Serializer serializer;
	blockHeader.Serialize(serializer);

	Slice key((const char*)&hash[0], hash.size());
	Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());
	m_pDatabase->Put(WriteOptions(), Slice(key), value);
}

void BlockDB::AddBlockHeaders(const std::vector<BlockHeader*>& blockHeaders)
{
	LoggerAPI::LogInfo("BlockDB::AddBlockHeaders - Adding headers - " + std::to_string(blockHeaders.size()));
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	for (const BlockHeader* pBlockHeader : blockHeaders)
	{
		const std::vector<unsigned char>& hash = pBlockHeader->GetHash().GetData();

		Serializer serializer;
		pBlockHeader->Serialize(serializer);

		Slice key((const char*)&hash[0], hash.size());
		Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());
		m_pDatabase->Put(WriteOptions(), key, value);
	}

	LoggerAPI::LogInfo("BlockDB::AddBlockHeaders - Finished adding headers.");
}

void BlockDB::AddBlockSums(const Hash& blockHash, const BlockSums& blockSums)
{
	LoggerAPI::LogInfo("BlockDB::AddBlockSums - Adding BlockSums for block " + HexUtil::ConvertHash(blockHash));
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	// Calculate Key Value ("SUMS_" + <Hash_In_Hex_>)
	const std::string key = BLOCK_SUMS_KEY + HexUtil::ConvertToHex(blockHash.GetData(), false, false);
	const Slice keyValue((const char*)&key[0], key.size());

	// Serializes the BlockSums object
	Serializer serializer;
	blockSums.Serialize(serializer);
	Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());

	// Insert BlockSums object
	m_pDatabase->Put(WriteOptions(), keyValue, value);
}

std::unique_ptr<BlockSums> BlockDB::GetBlockSums(const Hash& blockHash)
{
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	std::unique_ptr<BlockSums> pBlockSums = std::unique_ptr<BlockSums>(nullptr);

	// Calculate Key Value ("SUMS_" + <Hash_In_Hex_>)
	const std::string key = BLOCK_SUMS_KEY + HexUtil::ConvertToHex(blockHash.GetData(), false, false);
	Slice keyValue((const char*)&key[0], key.size());

	// Read from DB
	std::string value;
	const Status s = m_pDatabase->Get(ReadOptions(), keyValue, &value);
	if (s.ok())
	{
		// Deserialize result
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(data);
		pBlockSums = std::make_unique<BlockSums>(BlockSums::Deserialize(byteBuffer));
	}

	return pBlockSums;
}


void BlockDB::AddOutputPosition(const Commitment& outputCommitment, const uint64_t mmrIndex)
{
	const std::string outputHex = HexUtil::ConvertToHex(outputCommitment.GetCommitmentBytes().GetData(), false, false);
	LoggerAPI::LogInfo(StringUtil::Format("BlockDB::AddOutputPosition - Adding position (%llu) for output (%s).", mmrIndex, outputHex.c_str()));

	std::lock_guard<std::mutex> lockGuard(m_mutex);

	// Calculate Key Value ("OUT_POS_" + <Commitment_In_Hex_>)
	const std::string key = OUTPUT_POS_KEY + outputHex;
	const Slice keyValue(&key[0], key.size());

	// Serializes the output position
	Serializer serializer;
	serializer.Append<uint64_t>(mmrIndex);
	Slice value((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size());

	// Insert the output position
	m_pDatabase->Put(WriteOptions(), keyValue, value);
}

std::optional<uint64_t> BlockDB::GetOutputPosition(const Commitment& outputCommitment)
{
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	std::optional<uint64_t> outputPosition = std::nullopt;

	// Calculate Key Value ("OUT_POS_" + <Commitment_In_Hex_>)
	const std::string key = OUTPUT_POS_KEY + HexUtil::ConvertToHex(outputCommitment.GetCommitmentBytes().GetData(), false, false);
	Slice keyValue((const char*)&key[0], key.size());

	// Read from DB
	std::string value;
	const Status s = m_pDatabase->Get(ReadOptions(), keyValue, &value);
	if (s.ok())
	{
		// Deserialize result
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(data);
		outputPosition = std::make_optional<uint64_t>(byteBuffer.ReadU64());
	}

	return outputPosition;
}