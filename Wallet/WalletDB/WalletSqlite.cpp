#include "WalletSqlite.h"
#include "WalletEncryptionUtil.h"
#include "SqliteTransaction.h"
#include "Tables/VersionTable.h"
#include "Tables/OutputsTable.h"
#include "Tables/TransactionsTable.h"
#include "Tables/MetadataTable.h"

#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

static const uint8_t ENCRYPTION_FORMAT = 0;
static const int LATEST_SCHEMA_VERSION = 1;

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
	return FileUtil::GetSubDirectories(m_config.GetWalletConfig().GetWalletDirectory());
}

bool WalletSqlite::OpenWallet(const std::string& username, const SecureVector& masterSeed)
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

		const int version = VersionTable::GetCurrentVersion(*pDatabase);
		if (version == 0)
		{
			SqliteTransaction transaction(*pDatabase);
			transaction.Begin();

			VersionTable::CreateTable(*pDatabase);
			OutputsTable::UpdateSchema(*pDatabase, masterSeed, version);
			MetadataTable::UpdateSchema(*pDatabase, version);

			transaction.Commit();
		}
		else if (version > LATEST_SCHEMA_VERSION)
		{
			throw WALLET_STORE_EXCEPTION("Sqlite database has higher version than supported.");
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
		LOG_WARNING("Wallet already exists for user: " + username);
		//LoggerAPI::LogWarning("WalletSqlite::CreateWallet - Wallet already exists for user: " + username);
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

bool WalletSqlite::AddOutputs(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& outputs)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::AddOutputs - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	// TODO: Batch write as a transaction

	OutputsTable::AddOutputs(*m_userDBs.at(username), masterSeed, outputs);
	return true;
}

std::vector<OutputData> WalletSqlite::GetOutputs(const std::string& username, const SecureVector& masterSeed) const
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::GetOutputs - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	return OutputsTable::GetOutputs(*m_userDBs.at(username), masterSeed);
}

bool WalletSqlite::AddTransaction(const std::string& username, const SecureVector& masterSeed, const WalletTx& walletTx)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::AddTransaction - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	TransactionsTable::AddTransactions(*m_userDBs.at(username), masterSeed, std::vector<WalletTx>({ walletTx }));
	return true;
}

std::vector<WalletTx> WalletSqlite::GetTransactions(const std::string& username, const SecureVector& masterSeed) const
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::GetTransactions - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	return TransactionsTable::GetTransactions(*m_userDBs.at(username), masterSeed);
}

uint32_t WalletSqlite::GetNextTransactionId(const std::string& username)
{
	UserMetadata metadata = GetMetadata(username);

	const uint32_t nextTxId = metadata.GetNextTxId();
	const UserMetadata updatedMetadata(nextTxId + 1, metadata.GetRefreshBlockHeight(), metadata.GetRestoreLeafIndex());
	SaveMetadata(username, updatedMetadata);

	return nextTxId;
}

uint64_t WalletSqlite::GetRefreshBlockHeight(const std::string& username) const
{
	UserMetadata metadata = GetMetadata(username);

	return metadata.GetRefreshBlockHeight();
}

bool WalletSqlite::UpdateRefreshBlockHeight(const std::string& username, const uint64_t refreshBlockHeight)
{
	UserMetadata metadata = GetMetadata(username);

	return SaveMetadata(username, UserMetadata(metadata.GetNextTxId(), refreshBlockHeight, metadata.GetRestoreLeafIndex()));
}

uint64_t WalletSqlite::GetRestoreLeafIndex(const std::string& username) const
{
	UserMetadata metadata = GetMetadata(username);

	return metadata.GetRestoreLeafIndex();
}

bool WalletSqlite::UpdateRestoreLeafIndex(const std::string& username, const uint64_t lastLeafIndex)
{
	UserMetadata metadata = GetMetadata(username);

	return SaveMetadata(username, UserMetadata(metadata.GetNextTxId(), metadata.GetRefreshBlockHeight(), lastLeafIndex));
}

UserMetadata WalletSqlite::GetMetadata(const std::string& username) const
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::GetMetadata - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	return MetadataTable::GetMetadata(*m_userDBs.at(username));
}

bool WalletSqlite::SaveMetadata(const std::string& username, const UserMetadata& userMetadata)
{
	if (m_userDBs.find(username) == m_userDBs.end())
	{
		LoggerAPI::LogError("WalletSqlite::SaveMetadata - Database not open for user: " + username);
		throw WALLET_STORE_EXCEPTION("Database not open");
	}

	MetadataTable::SaveMetadata(*m_userDBs.at(username), userMetadata);
	return true;
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

	try
	{
		SqliteTransaction transaction(*pDatabase);
		transaction.Begin();

		VersionTable::CreateTable(*pDatabase);
		OutputsTable::CreateTable(*pDatabase);
		TransactionsTable::CreateTable(*pDatabase);
		MetadataTable::CreateTable(*pDatabase);

		std::string tableCreation = "create table accounts(parent_path TEXT PRIMARY KEY, account_name TEXT NOT NULL, next_child_index INTEGER NOT NULL);";
		tableCreation += "create table slate_contexts(slate_id TEXT PRIMARY KEY, enc_blind BLOB NOT NULL, enc_nonce BLOB NOT NULL);";
		// TODO: Add indices

		KeyChainPath nextChildPath = KeyChainPath::FromString("m/0/0").GetRandomChild();
		tableCreation += "insert into accounts values('m/0/0','DEFAULT'," + std::to_string(nextChildPath.GetKeyIndices().back()) + ");";

		char* error = nullptr;
		if (sqlite3_exec(pDatabase, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
		{
			LoggerAPI::LogError("WalletSqlite::CreateWallet - Failed to create DB tables for user: " + username);
			sqlite3_free(error);
		}

		transaction.Commit();
	}
	catch (WalletStoreException&)
	{
		sqlite3_close(pDatabase);
		FileUtil::RemoveFile(walletDBFile);

		return nullptr;
	}

	return pDatabase;
}