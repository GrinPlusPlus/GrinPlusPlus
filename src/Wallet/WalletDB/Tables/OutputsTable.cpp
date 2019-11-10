#include "OutputsTable.h"
#include "../WalletEncryptionUtil.h"

#include <Infrastructure/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>

// TABLE: outputs
// id: INTEGER PRIMARY KEY
// commitment: TEXT NOT NULL
// status: INTEGER NOT NULL
// transaction_id: INTEGER
// encrypted: BLOB NOT NULL
void OutputsTable::CreateTable(sqlite3& database)
{
	const std::string tableCreation = "create table outputs(id INTEGER PRIMARY KEY, commitment TEXT UNIQUE NOT NULL, status INTEGER NOT NULL, transaction_id INTEGER, encrypted BLOB NOT NULL);";
	char* error = nullptr;
	if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("OutputsTable::CreateTable - Create outputs table failed with error: %s", error));
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Error creating outputs table.");
	}
}

void OutputsTable::UpdateSchema(sqlite3& database, const SecureVector& masterSeed, const int previousVersion)
{
	if (previousVersion >= 1)
	{
		return;
	}

	// Create "new_outputs" table
	const std::string tableCreation = "create table new_outputs(id INTEGER PRIMARY KEY, commitment TEXT UNIQUE NOT NULL, status INTEGER NOT NULL, transaction_id INTEGER, encrypted BLOB NOT NULL);";
	char* error = nullptr;
	if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("OutputsTable::UpdateSchema - Create new_outputs table failed with error: %s", error));
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Error creating new_outputs table.");
	}

	// Load all outputs from existing table
	std::vector<OutputData> outputs = GetOutputs(database, masterSeed, previousVersion);

	// Add outputs to "new_outputs" table
	AddOutputs(database, masterSeed, outputs, "new_outputs");

	// Delete existing table
	const std::string dropTable = "DROP TABLE outputs";
	if (sqlite3_exec(&database, dropTable.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("OutputsTable::UpdateSchema - Dropping outputs table failed with error: %s", error));
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Error dropping outputs table.");
	}

	// Rename "new_outputs" table to "outputs"
	const std::string renameTable = "ALTER TABLE new_outputs RENAME TO outputs";
	if (sqlite3_exec(&database, renameTable.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("OutputsTable::UpdateSchema - Renaming new_outputs table failed with error: %s", error));
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Error renaming new_outputs table.");
	}
}

void OutputsTable::AddOutputs(sqlite3& database, const SecureVector& masterSeed, const std::vector<OutputData>& outputs)
{
	AddOutputs(database, masterSeed, outputs, "outputs");
}

void OutputsTable::AddOutputs(sqlite3& database, const SecureVector& masterSeed, const std::vector<OutputData>& outputs, const std::string& tableName)
{
	for (const OutputData& output : outputs)
	{
		sqlite3_stmt* stmt = nullptr;
		std::string insert = "insert into " + tableName + "(commitment, status, transaction_id, encrypted) values(?, ?, ?, ?)";
		insert += " ON CONFLICT(commitment) DO UPDATE SET status=excluded.status, transaction_id=excluded.transaction_id, encrypted=excluded.encrypted";
		if (sqlite3_prepare_v2(&database, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
		{
			LoggerAPI::LogError(StringUtil::Format("WalletSqlite::AddOutputs - Error while compiling sql: %s", sqlite3_errmsg(&database)));
			sqlite3_finalize(stmt);
			throw WALLET_STORE_EXCEPTION("Error compiling statement.");
		}

		const std::string commitmentHex = output.GetOutput().GetCommitment().ToHex();
		sqlite3_bind_text(stmt, 1, commitmentHex.c_str(), (int)commitmentHex.size(), NULL);

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
		const std::vector<unsigned char> encrypted = WalletEncryptionUtil::Encrypt(masterSeed, "OUTPUT", serializer.GetSecureBytes());
		sqlite3_bind_blob(stmt, 4, (const void*)encrypted.data(), (int)encrypted.size(), NULL);

		sqlite3_step(stmt);

		if (sqlite3_finalize(stmt) != SQLITE_OK)
		{
			LoggerAPI::LogError(StringUtil::Format("WalletSqlite::AddOutputs - Error finalizing statement: %s", sqlite3_errmsg(&database)));
			throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
		}
	}
}

std::vector<OutputData> OutputsTable::GetOutputs(sqlite3& database, const SecureVector& masterSeed)
{
	return GetOutputs(database, masterSeed, 1);
}

std::vector<OutputData> OutputsTable::GetOutputs(sqlite3& database, const SecureVector& masterSeed, const int version)
{
	// Prepare statement
	sqlite3_stmt* stmt = nullptr;
	const std::string query = "select encrypted from outputs";
	if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("SqliteUpdater::ConvertOutputs - Error while compiling sql: %s", sqlite3_errmsg(&database)));
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
		const SecureVector decrypted = WalletEncryptionUtil::Decrypt(masterSeed, "OUTPUT", encrypted);
		const std::vector<unsigned char> decryptedUnsafe(decrypted.begin(), decrypted.end());

		ByteBuffer byteBuffer(std::move(decryptedUnsafe));
		outputs.emplace_back(OutputData::Deserialize(byteBuffer));
	}

	if (ret_code != SQLITE_DONE)
	{
		LoggerAPI::LogError(StringUtil::Format("OutputsTable::GetOutputs - Error while performing sql: %s", sqlite3_errmsg(&database)));
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		LoggerAPI::LogError(StringUtil::Format("OutputsTable::GetOutputs - Error finalizing statement: %s", sqlite3_errmsg(&database)));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return outputs;
}