#include "MetadataTable.h"
#include "../SqliteDB.h"

#include <Common/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>

// TABLE: metadata
// id: INTEGER PRIMARY KEY
// next_tx_id: INTEGER NOT NULL
// refresh_block_height: BLOB NOT NULL
// restore_leaf_index: INTEGER NOT NULL
void MetadataTable::CreateTable(SqliteDB& database)
{
	std::string table_creation_cmd = "create table metadata(id INTEGER PRIMARY KEY, next_tx_id INTEGER NOT NULL, refresh_block_height INTEGER NOT NULL, restore_leaf_index INTEGER NOT NULL);";
	table_creation_cmd += "insert into metadata values(1, 0, 0, 0);";
	
	database.Execute(table_creation_cmd);
}

void MetadataTable::UpdateSchema(SqliteDB& database, const int previousVersion)
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

UserMetadata MetadataTable::GetMetadata(SqliteDB& database)
{
	std::string get_metadata_query = "SELECT next_tx_id, refresh_block_height, restore_leaf_index FROM metadata WHERE ID=1";
	auto pStatement = database.Query(get_metadata_query);

	if (pStatement->Step()) {
		const uint32_t nextTxId = (uint32_t)pStatement->GetColumnInt(0);
		const uint64_t refreshBlockHeight = (uint64_t)pStatement->GetColumnInt64(1);
		const uint64_t restoreLeafIndex = (uint64_t)pStatement->GetColumnInt64(2);

		return UserMetadata(nextTxId, refreshBlockHeight, restoreLeafIndex);
	} else {
		WALLET_ERROR_F("Error while performing sql: {}", database.GetError());
		throw WALLET_STORE_EXCEPTION("No metadata found.");
	}
}

void MetadataTable::SaveMetadata(SqliteDB& database, const UserMetadata& userMetadata)
{
	std::string update_metadata_cmd = StringUtil::Format(
		"update metadata set next_tx_id={}, refresh_block_height={}, restore_leaf_index={} where id=1;",
		userMetadata.GetNextTxId(),
		userMetadata.GetRefreshBlockHeight(),
		userMetadata.GetRestoreLeafIndex()
	);
	database.Execute(update_metadata_cmd);
}