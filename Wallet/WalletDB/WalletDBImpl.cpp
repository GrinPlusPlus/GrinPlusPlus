#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "WalletDBImpl.h"

#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Util/FileUtil.h>
#include <Infrastructure/Logger.h>

static ColumnFamilyDescriptor SEED_COLUMN = ColumnFamilyDescriptor("SEED", *ColumnFamilyOptions().OptimizeForPointLookup(1024));
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

	// open DB // TODO: Create one DB per username?
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
		m_pDatabase->CreateColumnFamily(SEED_COLUMN.options, SEED_COLUMN.name, &m_pSeedHandle);
		m_pDatabase->CreateColumnFamily(NEXT_CHILD_COLUMN.options, NEXT_CHILD_COLUMN.name, &m_pNextChildHandle);
		m_pDatabase->CreateColumnFamily(LOG_COLUMN.options, LOG_COLUMN.name, &m_pLogHandle);
		m_pDatabase->CreateColumnFamily(SLATE_COLUMN.options, SLATE_COLUMN.name, &m_pSlateHandle);
		m_pDatabase->CreateColumnFamily(TX_COLUMN.options, TX_COLUMN.name, &m_pTxHandle);
		m_pDatabase->CreateColumnFamily(OUTPUT_COLUMN.options, OUTPUT_COLUMN.name, &m_pOutputHandle);
	}
	else
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor(), SEED_COLUMN, NEXT_CHILD_COLUMN, LOG_COLUMN, SLATE_COLUMN, TX_COLUMN, OUTPUT_COLUMN });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);
		m_pDefaultHandle = columnHandles[0];
		m_pSeedHandle = columnHandles[1];
		m_pNextChildHandle = columnHandles[2];
		m_pLogHandle = columnHandles[3];
		m_pSlateHandle = columnHandles[4];
		m_pTxHandle = columnHandles[5];
		m_pOutputHandle = columnHandles[6];
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
	if (LoadWalletSeed(username) != nullptr)
	{
		LoggerAPI::LogWarning("WalletDB::CreateWallet - Wallet already exists for user: " + username);
		return false;
	}

	const Slice key(username);

	Serializer serializer;
	encryptedSeed.Serialize(serializer);
	const Slice value(std::string((const char*)&serializer.GetBytes()[0], serializer.GetBytes().size()));

	const Status updateStatus = m_pDatabase->Put(WriteOptions(), m_pSeedHandle, key, value);
	if (updateStatus.ok())
	{
		return true;
	}

	return false;
}

std::unique_ptr<EncryptedSeed> WalletDB::LoadWalletSeed(const std::string& username) const
{
	const Slice key(username);

	std::string value;
	const Status readStatus = m_pDatabase->Get(ReadOptions(), m_pSeedHandle, key, &value);
	if (readStatus.ok())
	{
		const std::vector<unsigned char> bytes(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(bytes);
		return std::make_unique<EncryptedSeed>(EncryptedSeed::Deserialize(byteBuffer));
	}

	return std::unique_ptr<EncryptedSeed>(nullptr);
}

KeyChainPath WalletDB::GetNextChildPath(const std::string& username, const KeyChainPath& parentPath)
{
	KeyChainPath nextChildPath = parentPath.GetFirstChild();
	const Slice key(CombineKeyWithUsername(username, parentPath.ToString()));

	std::string value;
	const Status readStatus = m_pDatabase->Get(ReadOptions(), m_pNextChildHandle, key, &value);
	if (readStatus.ok())
	{
		nextChildPath = KeyChainPath::FromString(value);
	}
	
	// Update nextChild
	std::vector<uint32_t> keyIndices = nextChildPath.GetKeyIndices();
	const uint32_t nextChildIndex = keyIndices.back() + 1;
	keyIndices.pop_back();
	keyIndices.push_back(nextChildIndex);

	const std::string newValue = KeyChainPath(std::move(keyIndices)).ToString();
	const Status updateStatus = m_pDatabase->Put(WriteOptions(), m_pNextChildHandle, key, newValue);
	if (!updateStatus.ok())
	{
		throw WalletStoreException();
	}

	return nextChildPath;
}

std::unique_ptr<SlateContext> WalletDB::LoadSlateContext(const std::string& username, const CBigInteger<32>& masterSeed, const uuids::uuid& slateId) const
{
	const std::string slateIdStr = uuids::to_string(slateId);
	const Slice key(CombineKeyWithUsername(username, slateIdStr));

	std::string value;
	const Status readStatus = m_pDatabase->Get(ReadOptions(), m_pSlateHandle, key, &value);
	if (readStatus.ok())
	{
		const std::vector<unsigned char> slateContextBytes(value.data(), value.data() + value.size());
		return std::make_unique<SlateContext>(SlateContext::Decrypt(slateContextBytes, masterSeed, slateId));
	}

	return std::unique_ptr<SlateContext>(nullptr);
}

bool WalletDB::SaveSlateContext(const std::string& username, const CBigInteger<32>& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext)
{
	const std::string slateIdStr = uuids::to_string(slateId);
	const Slice key(CombineKeyWithUsername(username, slateIdStr));

	const std::vector<unsigned char> encrypted = slateContext.Encrypt(masterSeed, slateId);
	const Slice value(std::string((const char*)encrypted.data(), encrypted.size()));

	const Status updateStatus = m_pDatabase->Put(WriteOptions(), m_pSlateHandle, key, value);
	if (updateStatus.ok())
	{
		LoggerAPI::LogInfo("WalletDB::SaveSlateContext - Context saved for slate " + slateIdStr);
		return true;
	}

	LoggerAPI::LogError("WalletDB::SaveSlateContext - Failed to save context for slate " + slateIdStr);
	return false;
}

bool WalletDB::AddOutputs(const std::string& username, const CBigInteger<32>& masterSeed, const std::vector<OutputData>& outputs)
{
	WriteBatch batch;

	const std::string prefix = GetUsernamePrefix(username);
	for (const OutputData& output : outputs)
	{
		std::string keyStr = prefix + output.GetKeyChainPath().ToString();
		if (output.GetMMRIndex().has_value())
		{
			keyStr += "_" + std::to_string(output.GetMMRIndex().value());
		}

		const std::vector<unsigned char> encrypted = output.Encrypt(masterSeed);

		const Slice key(keyStr);
		const Slice value(std::string(encrypted.begin(), encrypted.end()));
		batch.Put(m_pOutputHandle, key, value);
	}

	return m_pDatabase->Write(WriteOptions(), &batch).ok();
}

std::vector<OutputData> WalletDB::GetOutputs(const std::string& username, const CBigInteger<32>& masterSeed) const
{
	std::vector<OutputData> outputs;

	const std::string prefix = GetUsernamePrefix(username);

	auto iter = m_pDatabase->NewIterator(ReadOptions(), m_pOutputHandle);
	for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next())
	{
		const std::vector<unsigned char> encrypted(iter->value().data(), iter->value().data() + iter->value().size());
		outputs.emplace_back(OutputData::Decrypt(masterSeed, encrypted));
	}

	return outputs;
}

std::string WalletDB::GetUsernamePrefix(const std::string& username)
{
	const std::vector<unsigned char> usernameBytes(username.cbegin(), username.cend());
	return HexUtil::ConvertToHex(Crypto::Blake2b(usernameBytes).GetData());
}

Slice WalletDB::CombineKeyWithUsername(const std::string& username, const std::string& key)
{
	return Slice(GetUsernamePrefix(username) + key);
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