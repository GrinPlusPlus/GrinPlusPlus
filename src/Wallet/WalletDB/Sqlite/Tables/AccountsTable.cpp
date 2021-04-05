#include "AccountsTable.h"
#include "../SqliteDB.h"
#include "../Schema.h"

#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>


void AccountsTable::CreateTable(SqliteDB& database)
{
	std::string table_creation_cmd = "create table accounts(parent_path TEXT PRIMARY KEY, account_name TEXT NOT NULL, next_child_index INTEGER NOT NULL, current_address_index INTEGER NOT NULL);";

	database.Execute(table_creation_cmd);
}

void AccountsTable::UpdateSchema(SqliteDB& database, const int previousVersion)
{
	if (previousVersion < 3) {
		CreateTable(database);
	} else if (previousVersion < 4) {
		WALLET_INFO("Migrating Accounts Table to new schema");

		// Create "new_outputs" table
		std::string table_creation_cmd = "create table new_accounts(parent_path TEXT PRIMARY KEY, account_name TEXT NOT NULL, next_child_index INTEGER NOT NULL, current_address_index INTEGER NOT NULL);";
		database.Execute(table_creation_cmd);

		// Load existing slates
		std::string get_slate_query = "SELECT parent_path, account_name, next_child_index FROM accounts";;
		auto pStatement = database.Query(get_slate_query);

		while (pStatement->Step()) {
			WALLET_DEBUG_F(
				"Migrating slate with id {} at stage {}",
				pStatement->GetColumnString(0),
				pStatement->GetColumnString(1)
			);

			std::string insert_slate_cmd = "insert into new_accounts values(?, ?, ?, ?)";

			std::vector<SqliteDB::IParameter::UPtr> parameters;
			parameters.push_back(TextParameter::New(pStatement->GetColumnString(0)));
			parameters.push_back(TextParameter::New(pStatement->GetColumnString(1)));
			parameters.push_back(IntParameter::New(pStatement->GetColumnInt(2)));
			parameters.push_back(IntParameter::New(0));

			database.Update(insert_slate_cmd, parameters);
		}

		// Delete existing table
		std::string drop_table_cmd = "DROP TABLE accounts";
		database.Execute(drop_table_cmd);

		// Rename "new_accounts" table to "slate"
		const std::string rename_table_cmd = "ALTER TABLE new_accounts RENAME TO accounts";
		database.Execute(rename_table_cmd);
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
		WALLET_ERROR("Account not found for user");
		throw WALLET_STORE_EXCEPTION("Account not found.");
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