#include "AccountsTable.h"
#include "../SqliteDB.h"
#include "../Schema.h"

#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>

// TABLE: accounts
// parent_path: TEXT PRIMARY KEY
// account_name: TEXT NOT NULL
// next_child_index: INTEGER NOT NULL
// current_address_index: INTEGER DEFAULT 0 NOT NULL
void AccountsTable::CreateTable(SqliteDB& database)
{
	std::string table_creation_cmd = "CREATE TABLE IF NOT EXISTS accounts(parent_path TEXT PRIMARY KEY, account_name TEXT NOT NULL, next_child_index INTEGER NOT NULL, current_address_index INTEGER DEFAULT 0 NOT NULL);";
	database.Execute(table_creation_cmd);
}

void AccountsTable::UpdateSchema(SqliteDB& database, const int previousVersion)
{
	if (previousVersion < 2) {
		CreateTable(database);
	} else if(previousVersion < 4) {
		try {
			std::string table_creation_cmd = "ALTER TABLE accounts ADD current_address_index INTEGER DEFAULT 0 NOT NULL;";
			database.Execute(table_creation_cmd);
		} catch (...) { }
	}
}


int AccountsTable::GetNextChildIndex(SqliteDB& database, std::string parentPath)
{
	const std::string select_next_child_index = StringUtil::Format("SELECT next_child_index FROM accounts WHERE parent_path='{}'", parentPath);
	auto pStatement = database.Query(select_next_child_index);
	if (!pStatement->Step()) {
		WALLET_ERROR("Account not found for user");
		throw WALLET_STORE_EXCEPTION("Account not found.");
	}

	return pStatement->GetColumnInt(0);
}

void AccountsTable::UpdateNextChildIndex(SqliteDB& database, std::string parentPath, int index)
{
	std::string update_next_child_cmd = StringUtil::Format(
		"UPDATE accounts SET next_child_index={} WHERE parent_path='{}';",
		index,
		parentPath
	);

	database.Execute(update_next_child_cmd);
}


int AccountsTable::GetCurrentAddressIndex(SqliteDB& database, std::string parentPath)
{
	const std::string select_version_query = StringUtil::Format("SELECT current_address_index FROM accounts WHERE parent_path='{}'", parentPath);

	auto pStatement = database.Query(select_version_query);
	if (!pStatement->Step()) {
		WALLET_ERROR("Account not found at: " + parentPath);
		throw WALLET_STORE_EXCEPTION("Account not found at: " + parentPath);
	}

	return pStatement->GetColumnInt(0);
}

void AccountsTable::UpdateCurrentAddressIndex(SqliteDB& database, std::string parentPath, int index)
{
	std::string update_current_address_index_cmd = StringUtil::Format(
		"UPDATE accounts SET current_address_index={} WHERE parent_path='{}';",
		index,
		parentPath
	);

	database.Execute(update_current_address_index_cmd);
}
