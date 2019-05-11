#include "WalletDBImpl.h"

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
	ghc::filesystem::create_directories(dbPath);

	std::vector<std::string> columnFamilies;
	Status listStatus = DB::ListColumnFamilies(options, dbPath, &columnFamilies);
	if (!listStatus.ok() || columnFamilies.size() <= 1)
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor() });
		std::vector<ColumnFamilyHandle*> columnHandles;
		DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);

		m_pDefaultHandle = columnHandles[0];
		m_pDatabase->CreateColumnFamily(SEED_COLUMN.options, SEED_COLUMN.name, &m_pSeedHandle);
		m_pDatabase->CreateColumnFamily(NEXT_CHILD_COLUMN.options, NEXT_CHILD_COLUMN.name, &m_pNextChildHandle);
		m_pDatabase->CreateColumnFamily(LOG_COLUMN.options, LOG_COLUMN.name, &m_pLogHandle);
		m_pDatabase->CreateColumnFamily(SLATE_COLUMN.options, SLATE_COLUMN.name, &m_pSlateHandle);
		m_pDatabase->CreateColumnFamily(TX_COLUMN.options, TX_COLUMN.name, &m_pTxHandle);
		m_pDatabase->CreateColumnFamily(OUTPUT_COLUMN.options, OUTPUT_COLUMN.name, &m_pOutputHandle);
		m_pDatabase->CreateColumnFamily(USER_METADATA_COLUMN.options, USER_METADATA_COLUMN.name, &m_pUserMetadataHandle);
	}
	else
	{
		std::vector<ColumnFamilyDescriptor> columnDescriptors({ ColumnFamilyDescriptor(), SEED_COLUMN, NEXT_CHILD_COLUMN, LOG_COLUMN, SLATE_COLUMN, TX_COLUMN, OUTPUT_COLUMN, USER_METADATA_COLUMN });
		std::vector<ColumnFamilyHandle*> columnHandles;
		Status s = DB::Open(options, dbPath, columnDescriptors, &columnHandles, &m_pDatabase);
		m_pDefaultHandle = columnHandles[0];
		m_pSeedHandle = columnHandles[1];
		m_pNextChildHandle = columnHandles[2];
		m_pLogHandle = columnHandles[3];
		m_pSlateHandle = columnHandles[4];
		m_pTxHandle = columnHandles[5];
		m_pOutputHandle = columnHandles[6];
		m_pUserMetadataHandle = columnHandles[7];
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

std::vector<std::string> WalletDB::GetAccounts() const
{
	std::vector<std::string> usernames;

	auto iter = m_pDatabase->NewIterator(ReadOptions(), m_pSeedHandle);
	for (iter->SeekToFirst(); iter->Valid(); iter->Next())
	{
		usernames.emplace_back(iter->key().ToString());
	}

	return usernames;
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
	if (!updateStatus.ok())
	{
		return false;
	}

	return SaveMetadata(username, UserMetadata(0, 0, 0));
}

std::unique_ptr<EncryptedSeed> WalletDB::LoadWalletSeed(const std::string& username) const
{
	LoggerAPI::LogTrace("WalletDB::LoadWalletSeed - Loading wallet seed for " + username);

	const Slice key(username);

	std::string value;
	const Status readStatus = m_pDatabase->Get(ReadOptions(), m_pSeedHandle, key, &value);
	if (readStatus.ok())
	{
		LoggerAPI::LogTrace("WalletDB::LoadWalletSeed - Wallet seed loaded: " + value);

		const std::vector<unsigned char> bytes(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(bytes);
		return std::make_unique<EncryptedSeed>(EncryptedSeed::Deserialize(byteBuffer));
	}

	LoggerAPI::LogError("WalletDB::LoadWalletSeed - Wallet seed not found for " + username);
	return std::unique_ptr<EncryptedSeed>(nullptr);
}

KeyChainPath WalletDB::GetNextChildPath(const std::string& username, const KeyChainPath& parentPath)
{
	KeyChainPath nextChildPath = parentPath.GetRandomChild();
	const std::string keyWithUsername = CombineKeyWithUsername(username, parentPath.ToString());
	const Slice key(keyWithUsername);

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

std::unique_ptr<SlateContext> WalletDB::LoadSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId) const
{
	const std::string slateIdStr = uuids::to_string(slateId);
	const std::string keyWithUsername = CombineKeyWithUsername(username, slateIdStr);
	const Slice key(keyWithUsername);

	std::string value;
	const Status readStatus = m_pDatabase->Get(ReadOptions(), m_pSlateHandle, key, &value);
	if (readStatus.ok())
	{
		const std::vector<unsigned char> slateContextBytes(value.data(), value.data() + value.size());
		return std::make_unique<SlateContext>(SlateContext::Decrypt(slateContextBytes, masterSeed, slateId));
	}

	return std::unique_ptr<SlateContext>(nullptr);
}

bool WalletDB::SaveSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext)
{
	const std::string slateIdStr = uuids::to_string(slateId);
	const std::string keyWithUsername = CombineKeyWithUsername(username, slateIdStr);
	const Slice key(keyWithUsername);

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

bool WalletDB::AddOutputs(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& outputs)
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

		Serializer serializer;
		output.Serialize(serializer);
		const std::vector<unsigned char> encrypted = Encrypt(masterSeed, "OUTPUT", serializer.GetSecureBytes());

		const Slice key(keyStr);
		const Slice value((const char*)encrypted.data(), encrypted.size());
		batch.Put(m_pOutputHandle, key, value);
	}

	return m_pDatabase->Write(WriteOptions(), &batch).ok();
}

std::vector<OutputData> WalletDB::GetOutputs(const std::string& username, const SecureVector& masterSeed) const
{
	std::vector<OutputData> outputs;

	const std::string prefix = GetUsernamePrefix(username);

	auto iter = m_pDatabase->NewIterator(ReadOptions(), m_pOutputHandle);
	//for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next())
	for (iter->SeekToFirst(); iter->Valid(); iter->Next())
	{
		if (iter->key().starts_with(prefix))
		{
			const std::vector<unsigned char> encrypted(iter->value().data(), iter->value().data() + iter->value().size());
			const SecureVector decrypted = Decrypt(masterSeed, "OUTPUT", encrypted);
			const std::vector<unsigned char> decryptedUnsafe(decrypted.begin(), decrypted.end());

			ByteBuffer byteBuffer(decryptedUnsafe);
			outputs.emplace_back(OutputData::Deserialize(byteBuffer));
		}
	}

	return outputs;
}

bool WalletDB::AddTransaction(const std::string& username, const SecureVector& masterSeed, const WalletTx& walletTx)
{
	const std::string keyWithUsername = CombineKeyWithUsername(username, std::to_string(walletTx.GetId()));
	const Slice key(keyWithUsername);

	Serializer serializer;
	walletTx.Serialize(serializer);
	const std::vector<unsigned char> encrypted = Encrypt(masterSeed, "WALLET_TX", serializer.GetSecureBytes());
	const Slice value((const char*)encrypted.data(), encrypted.size());

	const Status updateStatus = m_pDatabase->Put(WriteOptions(), m_pTxHandle, key, value);
	if (updateStatus.ok())
	{
		LoggerAPI::LogInfo("WalletDB::AddTransaction - WalletTx saved with ID " + std::to_string(walletTx.GetId()));
		return true;
	}

	LoggerAPI::LogError("WalletDB::AddTransaction - Failed to save WalletTx with ID " + std::to_string(walletTx.GetId()));
	return false;
}

std::vector<WalletTx> WalletDB::GetTransactions(const std::string& username, const SecureVector& masterSeed) const
{
	std::vector<WalletTx> walletTransactions;

	const std::string prefix = GetUsernamePrefix(username);

	auto iter = m_pDatabase->NewIterator(ReadOptions(), m_pTxHandle);
	//for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next())
	for (iter->SeekToFirst(); iter->Valid(); iter->Next())
	{
		if (iter->key().starts_with(prefix))
		{
			const std::vector<unsigned char> encrypted(iter->value().data(), iter->value().data() + iter->value().size());
			const SecureVector decrypted = Decrypt(masterSeed, "WALLET_TX", encrypted);
			const std::vector<unsigned char> decryptedUnsafe(decrypted.begin(), decrypted.end());

			ByteBuffer byteBuffer(decryptedUnsafe);
			walletTransactions.emplace_back(WalletTx::Deserialize(byteBuffer));
		}
	}

	return walletTransactions;
}

uint32_t WalletDB::GetNextTransactionId(const std::string& username)
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		throw WalletStoreException();
	}

	const uint32_t nextTxId = pUserMetadata->GetNextTxId();
	const UserMetadata updatedMetadata(nextTxId + 1, pUserMetadata->GetRefreshBlockHeight(), pUserMetadata->GetRestoreLeafIndex());
	if (!SaveMetadata(username, updatedMetadata))
	{
		throw WalletStoreException();
	}

	return nextTxId;
}

uint64_t WalletDB::GetRefreshBlockHeight(const std::string& username) const
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		throw WalletStoreException();
	}

	return pUserMetadata->GetRefreshBlockHeight();
}

bool WalletDB::UpdateRefreshBlockHeight(const std::string& username, const uint64_t refreshBlockHeight)
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		throw WalletStoreException();
	}

	return SaveMetadata(username, UserMetadata(pUserMetadata->GetNextTxId(), refreshBlockHeight, pUserMetadata->GetRestoreLeafIndex()));
}

uint64_t WalletDB::GetRestoreLeafIndex(const std::string& username) const
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		throw WalletStoreException();
	}

	return pUserMetadata->GetRestoreLeafIndex();
}

bool WalletDB::UpdateRestoreLeafIndex(const std::string& username, const uint64_t lastLeafIndex)
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		throw WalletStoreException();
	}

	return SaveMetadata(username, UserMetadata(pUserMetadata->GetNextTxId(), pUserMetadata->GetRefreshBlockHeight(), lastLeafIndex));
}

std::unique_ptr<UserMetadata> WalletDB::GetMetadata(const std::string& username) const
{
	const Slice key(GetUsernamePrefix(username));
	std::string value;
	Status readStatus = m_pDatabase->Get(ReadOptions(), m_pUserMetadataHandle, key, &value);
	if (readStatus.ok())
	{
		const std::vector<unsigned char> bytes(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(bytes);
		return std::make_unique<UserMetadata>(UserMetadata::Deserialize(byteBuffer));
	}

	return std::unique_ptr<UserMetadata>(nullptr);
}

bool WalletDB::SaveMetadata(const std::string& username, const UserMetadata& userMetadata)
{
	const Slice key(GetUsernamePrefix(username));

	Serializer serializer;
	userMetadata.Serialize(serializer);
	const Slice value((const char*)serializer.GetBytes().data(), serializer.GetBytes().size());

	return m_pDatabase->Put(WriteOptions(), m_pUserMetadataHandle, key, value).ok();
}

std::string WalletDB::GetUsernamePrefix(const std::string& username)
{
	const std::vector<unsigned char> usernameBytes(username.cbegin(), username.cend());
	return HexUtil::ConvertToHex(Crypto::Blake2b(usernameBytes).GetData());
}

std::string WalletDB::CombineKeyWithUsername(const std::string& username, const std::string& key)
{
	return GetUsernamePrefix(username) + key;
}

SecretKey WalletDB::CreateSecureKey(const SecureVector& masterSeed, const std::string& dataType)
{
	SecureVector seedWithNonce(masterSeed.data(), masterSeed.data() + masterSeed.size());

	Serializer nonceSerializer;
	nonceSerializer.AppendVarStr(dataType);
	seedWithNonce.insert(seedWithNonce.end(), nonceSerializer.GetBytes().begin(), nonceSerializer.GetBytes().end());

	return Crypto::Blake2b((const std::vector<unsigned char>&)seedWithNonce);
}

std::vector<unsigned char> WalletDB::Encrypt(const SecureVector& masterSeed, const std::string& dataType, const SecureVector& bytes)
{
	const CBigInteger<32> randomNumber = RandomNumberGenerator::GenerateRandom32();
	const CBigInteger<16> iv = CBigInteger<16>(&randomNumber[0]);
	const SecretKey key = CreateSecureKey(masterSeed, dataType);

	const std::vector<unsigned char> encryptedBytes = Crypto::AES256_Encrypt(bytes, key, iv);

	Serializer serializer;
	serializer.Append<uint8_t>(ENCRYPTION_FORMAT);
	serializer.AppendBigInteger(iv);
	serializer.AppendByteVector(encryptedBytes);
	return serializer.GetBytes();
}

SecureVector WalletDB::Decrypt(const SecureVector& masterSeed, const std::string& dataType, const std::vector<unsigned char>& encrypted)
{
	ByteBuffer byteBuffer(encrypted);

	const uint8_t formatVersion = byteBuffer.ReadU8();
	if (formatVersion != ENCRYPTION_FORMAT)
	{
		throw DeserializationException();
	}

	const CBigInteger<16> iv = byteBuffer.ReadBigInteger<16>();
	const std::vector<unsigned char> encryptedBytes = byteBuffer.ReadVector(byteBuffer.GetRemainingSize());
	const SecretKey key = CreateSecureKey(masterSeed, dataType);

	return Crypto::AES256_Decrypt(encryptedBytes, key, iv);
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
		delete (WalletDB*)pWalletDB;
	}
}