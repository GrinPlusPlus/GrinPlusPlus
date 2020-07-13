#include "VersionTable.h"
#include "../Schema.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>

void VersionTable::CreateTable(sqlite3& database)
{
	std::string tableCreation = "create table version(schema_version INTEGER PRIMARY KEY);";
	tableCreation += "insert into version(schema_version) values (2)";

	char* error = nullptr;
	if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		WALLET_ERROR_F("Create version table failed with error: {}", error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to create version table.");
	}
}

void VersionTable::UpdateSchema(sqlite3& database, const int previousVersion)
{
	if (previousVersion == 0)
	{
		CreateTable(database);
	}
	else
	{
		std::string update = StringUtil::Format(
			"update version set schema_version={};",
			LATEST_SCHEMA_VERSION
		);

		char* error = nullptr;
		if (sqlite3_exec(&database, update.c_str(), NULL, NULL, &error) != SQLITE_OK)
		{
			WALLET_ERROR_F("Failed to update schema. Error: {}", error);
			sqlite3_free(error);
			throw WALLET_STORE_EXCEPTION("Failed to update schema.");
		}
	}
}

int VersionTable::GetCurrentVersion(sqlite3& database)
{
	if (!DoesTableExist(database))
	{
		return 0;
	}

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "SELECT schema_version FROM version";
	if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(&database));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	int version = 0;
	int ret_code = sqlite3_step(stmt);
	if (ret_code == SQLITE_ROW)
	{
		version = sqlite3_column_int(stmt, 0);
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return version;
}

bool VersionTable::DoesTableExist(sqlite3& database)
{
	sqlite3_stmt* stmt = nullptr;
	const std::string query = "PRAGMA table_info(version)";
	if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(&database));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	bool exists = false;
	int ret_code = sqlite3_step(stmt);
	if (ret_code == SQLITE_ROW)
	{
		exists = true;
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return exists;
}