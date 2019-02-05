#include "WalletDBImpl.h"

#include <FileUtil.h>
#include <Infrastructure/Logger.h>

static ColumnFamilyDescriptor NEXT_CHILD_COLUMN = ColumnFamilyDescriptor("NEXT_CHILD", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor LOG_COLUMN = ColumnFamilyDescriptor("LOG", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor SLATE_COLUMN = ColumnFamilyDescriptor("SLATE", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor TX_COLUMN = ColumnFamilyDescriptor("TX", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
static ColumnFamilyDescriptor OUTPUT_COLUMN = ColumnFamilyDescriptor("OUTPUT", *ColumnFamilyOptions().OptimizeForPointLookup(1024));

WalletDB::WalletDB(const Config& config)
	: m_config(config)
{
}

void WalletDB::Open()
{
	Options options;
	// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
	options.IncreaseParallelism();

	// create the DB if it's not already present
	options.create_if_missing = true;
	options.compression = kNoCompression;

	// open DB // TODO: Create one DB per username.
	const std::string dbPath = m_config.GetWalletConfig().GetWalletDirectory();
	std::filesystem::create_directories(dbPath);

	std::vector<std::string> columnFamilies;
	DB::ListColumnFamilies(options, dbPath, &columnFamilies);

	if (columnFamilies.size() <= 1)
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor() });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);

		m_pDefaultHandle = columnHandles[0];
		m_pDatabase->CreateColumnFamily(NEXT_CHILD_COLUMN.options, NEXT_CHILD_COLUMN.name, &m_pNextChildHandle);
		m_pDatabase->CreateColumnFamily(LOG_COLUMN.options, LOG_COLUMN.name, &m_pLogHandle);
		m_pDatabase->CreateColumnFamily(SLATE_COLUMN.options, SLATE_COLUMN.name, &m_pSlateHandle);
		m_pDatabase->CreateColumnFamily(TX_COLUMN.options, TX_COLUMN.name, &m_pTxHandle);
		m_pDatabase->CreateColumnFamily(OUTPUT_COLUMN.options, OUTPUT_COLUMN.name, &m_pOutputHandle);
	}
	else
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor(), NEXT_CHILD_COLUMN, LOG_COLUMN, SLATE_COLUMN, TX_COLUMN, OUTPUT_COLUMN });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);
		m_pDefaultHandle = columnHandles[0];
		m_pNextChildHandle = columnHandles[1];
		m_pLogHandle = columnHandles[2];
		m_pSlateHandle = columnHandles[3];
		m_pTxHandle = columnHandles[4];
		m_pOutputHandle = columnHandles[5];
	}
}

void WalletDB::Close()
{
	delete m_pDefaultHandle;
	delete m_pNextChildHandle;
	delete m_pLogHandle;
	delete m_pSlateHandle;
	delete m_pTxHandle;
	delete m_pOutputHandle;
	delete m_pDatabase;
}

bool WalletDB::CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed)
{
	const std::string& walletDirectory = m_config.GetWalletConfig().GetWalletDirectory();
	const EEnvironmentType environmentType = m_config.GetEnvironment().GetEnvironmentType();

	std::string seedFileName = walletDirectory + "wallet.seed";
	if (environmentType == EEnvironmentType::MAINNET)
	{
		seedFileName = walletDirectory + "mainnet.seed";
	}
	else if (environmentType == EEnvironmentType::FLOONET)
	{
		seedFileName = walletDirectory + "floonet.seed";
	}

	if (std::filesystem::exists(seedFileName))
	{
		LoggerAPI::LogWarning("WalletDB::CreateWallet - File already exists: " + seedFileName);
		return false;
	}

	Serializer serializer;
	encryptedSeed.Serialize(serializer);
	
	return FileUtil::SafeWriteToFile(seedFileName, serializer.GetBytes());
}

std::unique_ptr<EncryptedSeed> WalletDB::LoadWalletSeed(const std::string& username) const
{
	std::unique_ptr<EncryptedSeed> pEncryptedSeed = std::unique_ptr<EncryptedSeed>(nullptr);

	const std::string& walletDirectory = m_config.GetWalletConfig().GetWalletDirectory();
	const EEnvironmentType environmentType = m_config.GetEnvironment().GetEnvironmentType();

	std::string seedFileName = walletDirectory + "wallet.seed";
	if (environmentType == EEnvironmentType::MAINNET)
	{
		seedFileName = walletDirectory + "mainnet.seed";
	}
	else if (environmentType == EEnvironmentType::FLOONET)
	{
		seedFileName = walletDirectory + "floonet.seed";
	}

	std::vector<unsigned char> data;
	if (FileUtil::ReadFile(seedFileName, data))
	{
		ByteBuffer byteBuffer(data);
		pEncryptedSeed = std::make_unique<EncryptedSeed>(EncryptedSeed::Deserialize(byteBuffer));
	}

	return pEncryptedSeed;
}

KeyChainPath WalletDB::GetNextChildPath(const std::string& username, const KeyChainPath& parentPath)
{
	KeyChainPath nextChildPath = parentPath.GetFirstChild();
	Slice key(parentPath.ToString());
	std::string value;
	Status readStatus = m_pDatabase->Get(ReadOptions(), m_pNextChildHandle, key, &value);
	if (readStatus.ok())
	{
		nextChildPath = KeyChainPath::FromString(value);
	}
	
	// Update nextChild
	std::vector<uint32_t> keyIndices = nextChildPath.GetKeyIndices();
	const uint32_t nextChildIndex = keyIndices.back() + 1;
	keyIndices.pop_back();
	keyIndices.push_back(nextChildIndex);

	std::string newValue = KeyChainPath(std::move(keyIndices)).ToString();
	Status updateStatus = m_pDatabase->Put(WriteOptions(), m_pNextChildHandle, key, newValue);
	if (!updateStatus.ok())
	{
		// TODO: Throw exception?
	}

	return nextChildPath;
}

bool WalletDB::SaveSlateContext(const std::string& username, const uuids::uuid& slateId, const SlateContext& slateContext)
{
	Slice key(uuids::to_string(slateId));

	Serializer serializer;
	slateContext.Serialize(serializer);
	Slice value(std::string((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size()));

	Status updateStatus = m_pDatabase->Put(WriteOptions(), m_pNextChildHandle, key, value);
	if (updateStatus.ok())
	{
		return true;
	}

	LoggerAPI::LogError("WalletDB::SaveSlateContext - Failed to save slate context.");
	return false;
}

bool WalletDB::AddOutputs(const std::string& username, const std::vector<OutputData>& outputs)
{
	return true;
}

std::vector<OutputData> WalletDB::GetOutputs(const std::string& username) const
{
	return std::vector<OutputData>();
}

namespace WalletDBAPI
{
	//
	// Opens the wallet database and returns an instance of IWalletDB.
	//
	WALLET_DB_API IWalletDB* OpenWalletDB(const Config& config)
	{
		WalletDB* pWalletDB = new WalletDB(config);
		pWalletDB->Open();

		return pWalletDB;
	}

	//
	// Closes the wallet database and cleans up the memory of IWalletDB.
	//
	WALLET_DB_API void CloseWalletDB(IWalletDB* pWalletDB)
	{
		((WalletDB*)pWalletDB)->Close();
		delete pWalletDB;
	}
}