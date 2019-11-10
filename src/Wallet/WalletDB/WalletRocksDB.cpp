#include "WalletRocksDB.h"

#include <Common/Exceptions/UnimplementedException.h>
#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Util/FileUtil.h>
#include <Infrastructure/Logger.h>

static const uint8_t ENCRYPTION_FORMAT = 0;

static ColumnFamilyDescriptor SEED_COLUMN = ColumnFamilyDescriptor("SEED", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor NEXT_CHILD_COLUMN = ColumnFamilyDescriptor("NEXT_CHILD", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor LOG_COLUMN = ColumnFamilyDescriptor("LOG", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor SLATE_COLUMN = ColumnFamilyDescriptor("SLATE", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor TX_COLUMN = ColumnFamilyDescriptor("TX", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor OUTPUT_COLUMN = ColumnFamilyDescriptor("OUTPUT", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor USER_METADATA_COLUMN = ColumnFamilyDescriptor("METADATA", *ColumnFamilyOptions().OptimizeForPointLookup(1024));

WalletRocksDB::WalletRocksDB(
	DB* pDatabase,
	ColumnFamilyHandle* pDefaultHandle,
	ColumnFamilyHandle* pSeedHandle,
	ColumnFamilyHandle* pNextChildHandle,
	ColumnFamilyHandle* pLogHandle,
	ColumnFamilyHandle* pSlateHandle,
	ColumnFamilyHandle* pTxHandle,
	ColumnFamilyHandle* pOutputHandle,
	ColumnFamilyHandle* pUserMetadataHandle)
	: m_pDatabase(pDatabase),
	m_pDefaultHandle(pDefaultHandle),
	m_pSeedHandle(pSeedHandle),
	m_pNextChildHandle(pNextChildHandle),
	m_pLogHandle(pLogHandle),
	m_pSlateHandle(pSlateHandle),
	m_pTxHandle(pTxHandle),
	m_pOutputHandle(pOutputHandle),
	m_pUserMetadataHandle(pUserMetadataHandle)
{

}

WalletRocksDB::~WalletRocksDB()
{
	delete m_pDefaultHandle;
	delete m_pNextChildHandle;
	delete m_pLogHandle;
	delete m_pSlateHandle;
	delete m_pTxHandle;
	delete m_pOutputHandle;
	delete m_pDatabase;
}

std::shared_ptr<WalletRocksDB> WalletRocksDB::Open(const Config& config)
{
	Options options;
	// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
	options.IncreaseParallelism();
	options.max_open_files = 10;

	// create the DB if it's not already present
	options.create_if_missing = true;
	options.compression = kNoCompression;

	// open DB // TODO: Create one DB per username?
	const std::string dbPath = config.GetWalletConfig().GetWalletDirectory();
	fs::create_directories(dbPath);

	rocksdb::DB* pDatabase = nullptr;
	rocksdb::ColumnFamilyHandle* pDefaultHandle;
	rocksdb::ColumnFamilyHandle* pSeedHandle;
	rocksdb::ColumnFamilyHandle* pNextChildHandle;
	rocksdb::ColumnFamilyHandle* pLogHandle;
	rocksdb::ColumnFamilyHandle* pSlateHandle;
	rocksdb::ColumnFamilyHandle* pTxHandle;
	rocksdb::ColumnFamilyHandle* pOutputHandle;
	rocksdb::ColumnFamilyHandle* pUserMetadataHandle;

	std::vector<std::string> columnFamilies;
	Status listStatus = DB::ListColumnFamilies(options, dbPath, &columnFamilies);
	if (!listStatus.ok() || columnFamilies.size() <= 1)
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor() });
		std::vector<ColumnFamilyHandle*> columnHandles;
		DB::Open(options, dbPath, columnDescriptors, &columnHandles, &pDatabase);

		pDefaultHandle = columnHandles[0];
		pDatabase->CreateColumnFamily(SEED_COLUMN.options, SEED_COLUMN.name, &pSeedHandle);
		pDatabase->CreateColumnFamily(NEXT_CHILD_COLUMN.options, NEXT_CHILD_COLUMN.name, &pNextChildHandle);
		pDatabase->CreateColumnFamily(LOG_COLUMN.options, LOG_COLUMN.name, &pLogHandle);
		pDatabase->CreateColumnFamily(SLATE_COLUMN.options, SLATE_COLUMN.name, &pSlateHandle);
		pDatabase->CreateColumnFamily(TX_COLUMN.options, TX_COLUMN.name, &pTxHandle);
		pDatabase->CreateColumnFamily(OUTPUT_COLUMN.options, OUTPUT_COLUMN.name, &pOutputHandle);
		pDatabase->CreateColumnFamily(USER_METADATA_COLUMN.options, USER_METADATA_COLUMN.name, &pUserMetadataHandle);
	}
	else
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor(), SEED_COLUMN, NEXT_CHILD_COLUMN, LOG_COLUMN, SLATE_COLUMN, TX_COLUMN, OUTPUT_COLUMN, USER_METADATA_COLUMN });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &pDatabase);
		pDefaultHandle = columnHandles[0];
		pSeedHandle = columnHandles[1];
		pNextChildHandle = columnHandles[2];
		pLogHandle = columnHandles[3];
		pSlateHandle = columnHandles[4];
		pTxHandle = columnHandles[5];
		pOutputHandle = columnHandles[6];
		pUserMetadataHandle = columnHandles[7];
	}

	return std::shared_ptr<WalletRocksDB>(new WalletRocksDB(
		pDatabase,
		pDefaultHandle,
		pSeedHandle,
		pNextChildHandle,
		pLogHandle,
		pSlateHandle,
		pTxHandle,
		pOutputHandle,
		pUserMetadataHandle
	));
}

std::vector<std::string> WalletRocksDB::GetAccounts() const
{
	std::vector<std::string> usernames;

	auto iter = m_pDatabase->NewIterator(ReadOptions(), m_pSeedHandle);
	for (iter->SeekToFirst(); iter->Valid(); iter->Next())
	{
		usernames.emplace_back(iter->key().ToString());
	}

	return usernames;
}

bool WalletRocksDB::OpenWallet(const std::string& username, const SecureVector& masterSeed)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

bool WalletRocksDB::CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

std::unique_ptr<EncryptedSeed> WalletRocksDB::LoadWalletSeed(const std::string& username) const
{
	LoggerAPI::LogTrace("WalletRocksDB::LoadWalletSeed - Loading wallet seed for " + username);

	const Slice key(username);

	std::string value;
	const Status readStatus = m_pDatabase->Get(ReadOptions(), m_pSeedHandle, key, &value);
	if (readStatus.ok())
	{
		std::vector<unsigned char> bytes(value.data(), value.data() + value.size());
		LoggerAPI::LogError("WalletRocksDB::LoadWalletSeed -  Deserialized: " + HexUtil::ConvertToHex(bytes));
		ByteBuffer byteBuffer(std::move(bytes));
		return std::make_unique<EncryptedSeed>(EncryptedSeed::Deserialize(byteBuffer));
	}

	LoggerAPI::LogError("WalletRocksDB::LoadWalletSeed - Wallet seed not found for " + username);
	return std::unique_ptr<EncryptedSeed>(nullptr);
}

KeyChainPath WalletRocksDB::GetNextChildPath(const std::string& username, const KeyChainPath& parentPath)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

std::unique_ptr<SlateContext> WalletRocksDB::LoadSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId) const
{
	throw UNIMPLEMENTED_EXCEPTION;
}

bool WalletRocksDB::SaveSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

bool WalletRocksDB::AddOutputs(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& outputs)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

std::vector<OutputData> WalletRocksDB::GetOutputs(const std::string& username, const SecureVector& masterSeed) const
{
	throw UNIMPLEMENTED_EXCEPTION;
}

bool WalletRocksDB::AddTransaction(const std::string& username, const SecureVector& masterSeed, const WalletTx& walletTx)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

std::vector<WalletTx> WalletRocksDB::GetTransactions(const std::string& username, const SecureVector& masterSeed) const
{
	throw UNIMPLEMENTED_EXCEPTION;
}

uint32_t WalletRocksDB::GetNextTransactionId(const std::string& username)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

uint64_t WalletRocksDB::GetRefreshBlockHeight(const std::string& username) const
{
	throw UNIMPLEMENTED_EXCEPTION;
}

bool WalletRocksDB::UpdateRefreshBlockHeight(const std::string& username, const uint64_t refreshBlockHeight)
{
	throw UNIMPLEMENTED_EXCEPTION;
}

uint64_t WalletRocksDB::GetRestoreLeafIndex(const std::string& username) const
{
	throw UNIMPLEMENTED_EXCEPTION;
}

bool WalletRocksDB::UpdateRestoreLeafIndex(const std::string& username, const uint64_t lastLeafIndex)
{
	throw UNIMPLEMENTED_EXCEPTION;
}