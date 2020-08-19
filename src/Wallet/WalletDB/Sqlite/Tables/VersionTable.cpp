#include "VersionTable.h"
#include "../Schema.h"
#include "../SqliteDB.h"

#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>

void VersionTable::CreateTable(SqliteDB& database)
{
	std::string table_creation_cmd = "create table version(schema_version INTEGER PRIMARY KEY);";
	table_creation_cmd += "insert into version(schema_version) values (2)";

	database.Execute(table_creation_cmd);
}

void VersionTable::UpdateSchema(SqliteDB& database, const int previousVersion)
{
	if (previousVersion == 0) {
		CreateTable(database);
	} else {
		std::string update_version_cmd = StringUtil::Format(
			"update version set schema_version={};",
			LATEST_SCHEMA_VERSION
		);
		database.Execute(update_version_cmd);
	}
}

int VersionTable::GetCurrentVersion(SqliteDB& database)
{
	if (!DoesTableExist(database)) {
		return 0;
	}

	const std::string select_version_query = "SELECT schema_version FROM version";
	auto pStatement = database.Query(select_version_query);
	if (!pStatement->Step()) {
		return 0;
	}

	return pStatement->GetColumnInt(0);
}

bool VersionTable::DoesTableExist(SqliteDB& database)
{
	std::string table_info_query = "PRAGMA table_info(version)";
	auto pStatement = database.Query(table_info_query);
	return pStatement->Step();
}