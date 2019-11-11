#include "MetadataTable.h"

#include <Infrastructure/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>

// TABLE: metadata
// id: INTEGER PRIMARY KEY
// next_tx_id: INTEGER NOT NULL
// refresh_block_height: BLOB NOT NULL
// restore_leaf_index: INTEGER NOT NULL
void MetadataTable::CreateTable(sqlite3& database)
{
	std::string tableCreation = "create table metadata(id INTEGER PRIMARY KEY, next_tx_id INTEGER NOT NULL, refresh_block_height INTEGER NOT NULL, restore_leaf_index INTEGER NOT NULL);";
	tableCreation += "insert into metadata values(1, 0, 0, 0);";

	char* error = nullptr;
	if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		WALLET_ERROR_F("Create metadata table failed with error: %s", error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Error creating transactions table.");
	}
}

void MetadataTable::UpdateSchema(sqlite3& database, const int previousVersion)
{
	if (previousVersion == 0)
	{
		UserMetadata metadata = MetadataTable::GetMetadata(database);
		UserMetadata newMetadata(
			metadata.GetNextTxId(),
			metadata.GetRefreshBlockHeight(),
			0
		);
		SaveMetadata(database, newMetadata);
	}

	return;
}

UserMetadata MetadataTable::GetMetadata(sqlite3& database)
{
	UserMetadata metadata(0, 0, 0);

	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(&database, "SELECT next_tx_id, refresh_block_height, restore_leaf_index FROM metadata WHERE ID=1", -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: %s", sqlite3_errmsg(&database));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const uint32_t nextTxId = (uint32_t)sqlite3_column_int(stmt, 0);
		const uint64_t refreshBlockHeight = (uint64_t)sqlite3_column_int64(stmt, 1);
		const uint64_t restoreLeafIndex = (uint64_t)sqlite3_column_int64(stmt, 2);

		metadata = UserMetadata(nextTxId, refreshBlockHeight, restoreLeafIndex);
	}
	else
	{
		WALLET_ERROR_F("Error while performing sql: %s", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("No metadata found.");
	}


	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: %s", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return metadata;
}

void MetadataTable::SaveMetadata(sqlite3& database, const UserMetadata& userMetadata)
{
	std::string update = StringUtil::Format(
		"update metadata set next_tx_id=%llu, refresh_block_height=%llu, restore_leaf_index=%llu where id=1;",
		userMetadata.GetNextTxId(),
		userMetadata.GetRefreshBlockHeight(),
		userMetadata.GetRestoreLeafIndex()
	);

	char* error = nullptr;
	if (sqlite3_exec(&database, update.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		WALLET_ERROR_F("Failed to save metadata for user. Error: %s", error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to save metadata.");
	}
}