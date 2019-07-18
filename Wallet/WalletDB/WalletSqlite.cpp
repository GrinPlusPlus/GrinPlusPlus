#include "WalletSqlite.h"

#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

static const uint8_t ENCRYPTION_FORMAT = 0;

WalletSqlite::WalletSqlite(const Config& config)
	: m_config(config)
{
}

void WalletSqlite::Open()
{
	m_userDBs.clear();
}

void WalletSqlite::Close()
{
	for (auto userDB : m_userDBs)
	{
		sqlite3_close(userDB.second);
	}

	m_userDBs.clear();
}

std::vector<std::string> WalletSqlite::GetAccounts() const
{
	std::vector<std::string> usernames;

	// TODO: Implement

	return usernames;
}

bool WalletSqlite::OpenWallet(const std::string& username)
{
	if (m_userDBs.find(username) != m_userDBs.end())
	{
		LoggerAPI::LogDebug("WalletSqlite::OpenWallet - wallet.db already open for user: " + username);
		return false;
	}

	const std::string userDBPath = m_config.GetWalletConfig().GetWalletDirectory() + "/" + username;
	const std::string dbFile = userDBPath + "/wallet.db";
	if (FileUtil::Exists(dbFile))
	{
		sqlite3* pDatabase = nullptr;
		if (sqlite3_open(dbFile.c_str(), &pDatabase) != SQLITE_OK)
		{
			LoggerAPI::LogError("WalletSqlite::OpenWallet - Failed to open wallet.db for user: " + username + ". Error: " + std::string(sqlite3_errmsg(pDatabase)));
			sqlite3_close(pDatabase);

			throw WALLET_STORE_EXCEPTION("Failed to create wallet.db");
		}

		m_userDBs[username] = pDatabase;
	}
	else
	{
		sqlite3* pDatabase = CreateWalletDB(username);
		if (pDatabase == nullptr)
		{
			LoggerAPI::LogError("WalletSqlite::OpenWallet - Failed to create wallet.db for user: " + username);
			throw WALLET_STORE_EXCEPTION("Failed to create wallet.db");
		}

		m_userDBs[username] = pDatabase;
	}


	return true;
}

bool WalletSqlite::CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed)
{
	const std::string userDBPath = m_config.GetWalletConfig().GetWalletDirectory() + "/" + username;
	const std::string seedFile = userDBPath + "/seed.json";
	if (FileUtil::Exists(seedFile))
	{
		LoggerAPI::LogWarning("WalletSqlite::CreateWallet - Wallet already exists for user: " + username);
		return false;
	}

	if (!FileUtil::CreateDirectories(userDBPath))
	{
		LoggerAPI::LogError("WalletSqlite::CreateWallet - Failed to create wallet directory for user: " + username);
		throw WALLET_STORE_EXCEPTION("Failed to create directory.");
	}

	const std::string seedJSON = encryptedSeed.ToJSON().toStyledString();
	const std::vector<unsigned char> seedBytes(seedJSON.data(), seedJSON.data() + seedJSON.size());
	if (!FileUtil::SafeWriteToFile(seedFile, seedBytes))
	{
		LoggerAPI::LogError("WalletSqlite::CreateWallet - Failed to create seed.json for user: " + username);
		throw WALLET_STORE_EXCEPTION("Failed to create seed.json");
	}
	
	sqlite3* pDatabase = CreateWalletDB(username);
	if (pDatabase == nullptr)
	{
		LoggerAPI::LogError("WalletSqlite::OpenWallet - Failed to create wallet.db for user: " + username);
		FileUtil::RemoveFile(userDBPath);
		throw WALLET_STORE_EXCEPTION("Failed to create wallet.db");
	}

	m_userDBs[username] = pDatabase;

	return true;
}

std::unique_ptr<EncryptedSeed> WalletSqlite::LoadWalletSeed(const std::string& username) const
{
	LoggerAPI::LogTrace("WalletSqlite::LoadWalletSeed - Loading wallet seed for " + username);

	const std::string seedPath = m_config.GetWalletConfig().GetWalletDirectory() + "/" + username + "/seed.json";
	if (!FileUtil::Exists(seedPath))
	{
		LoggerAPI::LogWarning("Wallet not found for user: " + username);
		return std::unique_ptr<EncryptedSeed>(nullptr);
	}

	std::vector<unsigned char> seedBytes;
	if (!FileUtil::ReadFile(seedPath, seedBytes))
	{
		LoggerAPI::LogError("WalletSqlite::LoadWalletSeed - Failed to load seed.json for user: " + username);
		throw WALLET_STORE_EXCEPTION("Failed to load seed.json");
	}

	std::string seed(seedBytes.data(), seedBytes.data() + seedBytes.size());

	Json::Value seedJSON;
	if (!JsonUtil::Parse(seed, seedJSON))
	{
		LoggerAPI::LogError("WalletSqlite::LoadWalletSeed - Failed to deserialize seed.json for user: " + username);
		throw WALLET_STORE_EXCEPTION("Failed to deserialize seed.json");
	}

	return std::make_unique<EncryptedSeed>(EncryptedSeed::FromJSON(seedJSON));
}

KeyChainPath WalletSqlite::GetNextChildPath(const std::string& username, const KeyChainPath& parentPath)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::GetNextChildPath - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	sqlite3* pDatabase = m_userDBs.at(username);

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "SELECT next_child_index FROM accounts WHERE parent_path='" + parentPath.ToString() + "'";
	if (sqlite3_prepare_v2(m_userDBs.at(username), query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetNextChildPath - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		LoggerAPI::LogError("WalletSqlite::GetNextChildPath - Account not found for user " + username);
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Account not found.");
	}

	const uint32_t nextChildIndex = (uint32_t)sqlite3_column_int(stmt, 0);
	KeyChainPath nextChildPath = parentPath.GetChild(nextChildIndex);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetNextChildPath - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	char* error = nullptr;
	const std::string update = "UPDATE accounts SET next_child_index=" + std::to_string(nextChildPath.GetKeyIndices().back() + 1) + " WHERE parent_path='" + parentPath.ToString() + "';";
	if (sqlite3_exec(pDatabase, update.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetNextChildPath - Failed to update account for user: %s. Error: %s", username.c_str(), error));
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to update account");
	}

	return nextChildPath;
}

std::unique_ptr<SlateContext> WalletSqlite::LoadSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId) const
{
	std::unique_ptr<SlateContext> pSlateContext = nullptr;

	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::LoadSlateContext - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	sqlite3* pDatabase = m_userDBs.at(username);

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "SELECT enc_blind, enc_nonce FROM slate_contexts WHERE slate_id='" + uuids::to_string(slateId) + "'";
	if (sqlite3_prepare_v2(m_userDBs.at(username), query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::LoadSlateContext - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		LoggerAPI::LogDebug("WalletSqlite::LoadSlateContext - Slate context found for user " + username);

		const int blindBytes = sqlite3_column_bytes(stmt, 0);
		const int nonceBytes = sqlite3_column_bytes(stmt, 1);
		if (blindBytes != 32 || nonceBytes != 32)
		{
			LoggerAPI::LogError("WalletSqlite::LoadSlateContext - Blind or nonce not 32 bytes.");
			sqlite3_finalize(stmt);
			throw WALLET_STORE_EXCEPTION("Slate context corrupt");
		}

		CBigInteger<32> encryptedBlindingFactor((const unsigned char*)sqlite3_column_blob(stmt, 0));
		SecretKey blindingFactor = encryptedBlindingFactor ^ SlateContext::DeriveXORKey(masterSeed, slateId, "blind");

		CBigInteger<32> encryptedNonce((const unsigned char*)sqlite3_column_blob(stmt, 1));
		SecretKey nonce = encryptedNonce ^ SlateContext::DeriveXORKey(masterSeed, slateId, "nonce");

		pSlateContext = std::make_unique<SlateContext>(SlateContext(std::move(blindingFactor), std::move(nonce)));
	}
	else
	{
		LoggerAPI::LogInfo("WalletSqlite::LoadSlateContext - Slate context not found for user " + username);
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::LoadSlateContext - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return pSlateContext;
}

bool WalletSqlite::SaveSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::LoadSlateContext - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	sqlite3* pDatabase = m_userDBs.at(username);

	sqlite3_stmt* stmt = nullptr;
	std::string insert = "insert into slate_contexts values(?, ?, ?)";
	if (sqlite3_prepare_v2(pDatabase, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::SaveSlateContext - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	const std::string slateIdStr = uuids::to_string(slateId);
	sqlite3_bind_text(stmt, 1, slateIdStr.c_str(), (int)slateIdStr.size(), NULL);

	CBigInteger<32> encryptedBlindingFactor = slateContext.GetSecretKey().GetBytes() ^ SlateContext::DeriveXORKey(masterSeed, slateId, "blind");
	sqlite3_bind_blob(stmt, 2, (const void*)encryptedBlindingFactor.data(), 32, NULL);

	CBigInteger<32> encryptedNonce = slateContext.GetSecretNonce().GetBytes() ^ SlateContext::DeriveXORKey(masterSeed, slateId, "nonce");
	sqlite3_bind_blob(stmt, 3, (const void*)encryptedNonce.data(), 32, NULL);

	sqlite3_step(stmt);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::SaveSlateContext - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return true;
}

// TABLE: outputs
// keychain_path: TEXT PRIMARY KEY
// status: INTEGER NOT NULL
// transaction_id: INTEGER
// encrypted: BLOB NOT NULL
bool WalletSqlite::AddOutputs(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& outputs)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::AddOutputs - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	// TODO: Batch write as a transaction

	sqlite3* pDatabase = m_userDBs.at(username);

	for (const OutputData& output : outputs)
	{
		sqlite3_stmt* stmt = nullptr;
		std::string insert = "insert into outputs(keychain_path, status, transaction_id, encrypted) values(?, ?, ?, ?)";
		insert += " ON CONFLICT(keychain_path) DO UPDATE SET status=excluded.status, transaction_id=excluded.transaction_id, encrypted=excluded.encrypted";
		if (sqlite3_prepare_v2(pDatabase, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
		{
			LoggerAPI::LogError(StringUtil::Format("WalletSqlite::AddOutputs - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
			sqlite3_finalize(stmt);
			throw WALLET_STORE_EXCEPTION("Error compiling statement.");
		}

		const std::string keychainPath = output.GetKeyChainPath().ToString();
		sqlite3_bind_text(stmt, 1, keychainPath.c_str(), (int)keychainPath.size(), NULL);

		sqlite3_bind_int(stmt, 2, (int)output.GetStatus());

		if (output.GetWalletTxId().has_value())
		{
			sqlite3_bind_int(stmt, 3, (int)output.GetWalletTxId().value());
		}
		else
		{
			sqlite3_bind_null(stmt, 3);
		}

		Serializer serializer;
		output.Serialize(serializer);
		const std::vector<unsigned char> encrypted = Encrypt(masterSeed, "OUTPUT", serializer.GetSecureBytes());
		sqlite3_bind_blob(stmt, 4, (const void*)encrypted.data(), (int)encrypted.size(), NULL);

		sqlite3_step(stmt);

		if (sqlite3_finalize(stmt) != SQLITE_OK)
		{
			LoggerAPI::LogError(StringUtil::Format("WalletSqlite::AddOutputs - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
			throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
		}
	}

	return true;
}

std::vector<OutputData> WalletSqlite::GetOutputs(const std::string& username, const SecureVector& masterSeed) const
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::GetOutputs - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	sqlite3* pDatabase = m_userDBs.at(username);

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "select encrypted from outputs";
	if (sqlite3_prepare_v2(m_userDBs.at(username), query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetOutputs - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	std::vector<OutputData> outputs;

	int ret_code = 0;
	while ((ret_code = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		const int encryptedSize = sqlite3_column_bytes(stmt, 0);
		const unsigned char* pEncrypted = (const unsigned char*)sqlite3_column_blob(stmt, 0);
		std::vector<unsigned char> encrypted(pEncrypted, pEncrypted + encryptedSize);
		const SecureVector decrypted = Decrypt(masterSeed, "OUTPUT", encrypted);
		const std::vector<unsigned char> decryptedUnsafe(decrypted.begin(), decrypted.end());

		ByteBuffer byteBuffer(decryptedUnsafe);
		outputs.emplace_back(OutputData::Deserialize(byteBuffer));
	}

	if (ret_code != SQLITE_DONE)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetOutputs - Error while performing sql: %s", sqlite3_errmsg(m_userDBs.at(username))));
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetOutputs - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return outputs;
}

// TABLE: transactions
// id: INTEGER PRIMARY KEY
// slate_id: TEXT
// encrypted: BLOB NOT NULL
bool WalletSqlite::AddTransaction(const std::string& username, const SecureVector& masterSeed, const WalletTx& walletTx)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::AddTransaction - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	// TODO: What if transaction already exists?

	sqlite3* pDatabase = m_userDBs.at(username);

	sqlite3_stmt* stmt = nullptr;
	std::string insert = "insert or replace into transactions values(?, ?, ?)";
	if (sqlite3_prepare_v2(pDatabase, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::AddTransaction - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	sqlite3_bind_int(stmt, 1, (int)walletTx.GetId());

	if (walletTx.GetSlateId().has_value())
	{
		const std::string slateId = uuids::to_string(walletTx.GetSlateId().value());
		sqlite3_bind_text(stmt, 2, slateId.c_str(), (int)slateId.size(), NULL);
	}
	else
	{
		sqlite3_bind_null(stmt, 2);
	}

	Serializer serializer;
	walletTx.Serialize(serializer);
	const std::vector<unsigned char> encrypted = Encrypt(masterSeed, "WALLET_TX", serializer.GetSecureBytes());
	sqlite3_bind_blob(stmt, 3, (const void*)encrypted.data(), (int)encrypted.size(), NULL);

	sqlite3_step(stmt);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::AddTransaction - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return true;
}

std::vector<WalletTx> WalletSqlite::GetTransactions(const std::string& username, const SecureVector& masterSeed) const
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::GetTransactions - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	sqlite3* pDatabase = m_userDBs.at(username);

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "select encrypted from transactions";
	if (sqlite3_prepare_v2(m_userDBs.at(username), query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetTransactions - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	std::vector<WalletTx> transactions;

	int ret_code = 0;
	while ((ret_code = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		const int encryptedSize = sqlite3_column_bytes(stmt, 0);
		const unsigned char* pEncrypted = (const unsigned char*)sqlite3_column_blob(stmt, 0);
		std::vector<unsigned char> encrypted(pEncrypted, pEncrypted + encryptedSize);
		const SecureVector decrypted = Decrypt(masterSeed, "WALLET_TX", encrypted);
		const std::vector<unsigned char> decryptedUnsafe(decrypted.begin(), decrypted.end());

		ByteBuffer byteBuffer(decryptedUnsafe);
		transactions.emplace_back(WalletTx::Deserialize(byteBuffer));
	}

	if (ret_code != SQLITE_DONE)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetTransactions - Error while performing sql: %s", sqlite3_errmsg(m_userDBs.at(username))));
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetTransactions - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return transactions;
}

uint32_t WalletSqlite::GetNextTransactionId(const std::string& username)
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		LoggerAPI::LogError("WalletSqlite::GetNextTransactionId - User metadata not found for user: " + username);
		throw WALLET_STORE_EXCEPTION("User metadata not found.");
	}

	const uint32_t nextTxId = pUserMetadata->GetNextTxId();
	const UserMetadata updatedMetadata(nextTxId + 1, pUserMetadata->GetRefreshBlockHeight(), pUserMetadata->GetRestoreLeafIndex());
	if (!SaveMetadata(username, updatedMetadata))
	{
		LoggerAPI::LogError("WalletSqlite::GetNextTransactionId - Failed to update user metadata for user: " + username);
		throw WALLET_STORE_EXCEPTION("Failed to update user metadata.");
	}

	return nextTxId;
}

uint64_t WalletSqlite::GetRefreshBlockHeight(const std::string& username) const
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		LoggerAPI::LogError("WalletSqlite::GetRefreshBlockHeight - User metadata not found for user: " + username);
		throw WALLET_STORE_EXCEPTION("User metadata not found.");
	}

	return pUserMetadata->GetRefreshBlockHeight();
}

bool WalletSqlite::UpdateRefreshBlockHeight(const std::string& username, const uint64_t refreshBlockHeight)
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		LoggerAPI::LogError("WalletSqlite::UpdateRefreshBlockHeight - User metadata not found for user: " + username);
		throw WALLET_STORE_EXCEPTION("User metadata not found.");
	}

	return SaveMetadata(username, UserMetadata(pUserMetadata->GetNextTxId(), refreshBlockHeight, pUserMetadata->GetRestoreLeafIndex()));
}

uint64_t WalletSqlite::GetRestoreLeafIndex(const std::string& username) const
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		LoggerAPI::LogError("WalletSqlite::GetRestoreLeafIndex - User metadata not found for user: " + username);
		throw WALLET_STORE_EXCEPTION("User metadata not found.");
	}

	return pUserMetadata->GetRestoreLeafIndex();
}

bool WalletSqlite::UpdateRestoreLeafIndex(const std::string& username, const uint64_t lastLeafIndex)
{
	std::unique_ptr<UserMetadata> pUserMetadata = GetMetadata(username);
	if (pUserMetadata == nullptr)
	{
		LoggerAPI::LogError("WalletSqlite::UpdateRestoreLeafIndex - User metadata not found for user: " + username);
		throw WALLET_STORE_EXCEPTION("User metadata not found.");
	}

	return SaveMetadata(username, UserMetadata(pUserMetadata->GetNextTxId(), pUserMetadata->GetRefreshBlockHeight(), lastLeafIndex));
}

std::unique_ptr<UserMetadata> WalletSqlite::GetMetadata(const std::string& username) const
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::GetNextChildPath - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	sqlite3* pDatabase = m_userDBs.at(username);

	std::unique_ptr<UserMetadata> pUserMetadata = nullptr;

	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(pDatabase, "SELECT next_tx_id, refresh_block_height, restore_leaf_index FROM metadata WHERE ID=1", -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetMetadata - Error while compiling sql: %s", sqlite3_errmsg(pDatabase)));
		sqlite3_finalize(stmt);
		return pUserMetadata;
	}

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const uint32_t nextTxId = (uint32_t)sqlite3_column_int(stmt, 0);
		const uint64_t refreshBlockHeight = (uint64_t)sqlite3_column_int64(stmt, 1);
		const uint64_t restoreLeafIndex = (uint64_t)sqlite3_column_int64(stmt, 2);

		pUserMetadata = std::make_unique<UserMetadata>(UserMetadata(nextTxId, refreshBlockHeight, restoreLeafIndex));
	}
	else
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetMetadata - Error while performing sql: %s", sqlite3_errmsg(pDatabase)));
	}


	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::GetMetadata - Error finalizing statement: %s", sqlite3_errmsg(pDatabase)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return pUserMetadata;
}

bool WalletSqlite::SaveMetadata(const std::string& username, const UserMetadata& userMetadata)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::SaveMetadata - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	sqlite3* pDatabase = m_userDBs.at(username);

	std::string update = StringUtil::Format(
		"update metadata set next_tx_id=%llu, refresh_block_height=%llu, restore_leaf_index=%llu where id=1;", 
		userMetadata.GetNextTxId(), 
		userMetadata.GetRefreshBlockHeight(), 
		userMetadata.GetRestoreLeafIndex()
	);

	char* error = nullptr;
	if (sqlite3_exec(pDatabase, update.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("WalletSqlite::SaveMetadata - Failed to save metadata for user: %s. Error: %s", username.c_str(), error));
		sqlite3_free(error);
		return false;
	}

	return true;
}

SecretKey WalletSqlite::CreateSecureKey(const SecureVector& masterSeed, const std::string& dataType)
{
	SecureVector seedWithNonce(masterSeed.data(), masterSeed.data() + masterSeed.size());

	Serializer nonceSerializer;
	nonceSerializer.AppendVarStr(dataType);
	seedWithNonce.insert(seedWithNonce.end(), nonceSerializer.GetBytes().begin(), nonceSerializer.GetBytes().end());

	return Crypto::Blake2b((const std::vector<unsigned char>&)seedWithNonce);
}

std::vector<unsigned char> WalletSqlite::Encrypt(const SecureVector& masterSeed, const std::string& dataType, const SecureVector& bytes)
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

SecureVector WalletSqlite::Decrypt(const SecureVector& masterSeed, const std::string& dataType, const std::vector<unsigned char>& encrypted)
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

sqlite3* WalletSqlite::CreateWalletDB(const std::string& username)
{
	const std::string walletDBFile = m_config.GetWalletConfig().GetWalletDirectory() + "/" + username + "/wallet.db";
	sqlite3* pDatabase = nullptr;
	if (sqlite3_open(walletDBFile.c_str(), &pDatabase) != SQLITE_OK)
	{
		LoggerAPI::LogError("WalletSqlite::CreateWallet - Failed to create wallet.db for user: " + username + ". Error: " + std::string(sqlite3_errmsg(pDatabase)));
		sqlite3_close(pDatabase);
		FileUtil::RemoveFile(walletDBFile);

		return nullptr;
	}

	std::string tableCreation = "create table metadata(id INTEGER PRIMARY KEY, next_tx_id INTEGER NOT NULL, refresh_block_height INTEGER NOT NULL, restore_leaf_index INTEGER NOT NULL);";
	tableCreation += "create table accounts(parent_path TEXT PRIMARY KEY, account_name TEXT NOT NULL, next_child_index INTEGER NOT NULL);";
	tableCreation += "create table transactions(id INTEGER PRIMARY KEY, slate_id TEXT, encrypted BLOB NOT NULL);";
	tableCreation += "create table outputs(keychain_path TEXT PRIMARY KEY, status INTEGER NOT NULL, transaction_id INTEGER, encrypted BLOB NOT NULL);";
	tableCreation += "create table slate_contexts(slate_id TEXT PRIMARY KEY, enc_blind BLOB NOT NULL, enc_nonce BLOB NOT NULL);";
	// TODO: Add indices

	KeyChainPath nextChildPath = KeyChainPath::FromString("m/0/0").GetRandomChild();
	tableCreation += "insert into accounts values('m/0/0','DEFAULT'," + std::to_string(nextChildPath.GetKeyIndices().back()) + ");";
	tableCreation += "insert into metadata values(1, 0, 0, 0);";

	char* error = nullptr;
	if (sqlite3_exec(pDatabase, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		LoggerAPI::LogError("WalletSqlite::CreateWallet - Failed to create DB tables for user: " + username);
		sqlite3_free(error);
		sqlite3_close(pDatabase);
		FileUtil::RemoveFile(walletDBFile);

		return nullptr;
	}

	return pDatabase;
}