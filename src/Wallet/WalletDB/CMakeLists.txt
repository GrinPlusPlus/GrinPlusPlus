set(TARGET_NAME WalletDB)

file(GLOB SOURCE_CODE
    "WalletDBImpl.cpp"
    "WalletSqlite.cpp"
	"WalletEncryptionUtil.cpp"
	"Sqlite/SqliteDB.cpp"
	"Sqlite/SqliteStore.cpp"
	"Sqlite/WalletSqlite.cpp"
	"Sqlite/Tables/MetadataTable.cpp"
	"Sqlite/Tables/OutputsTable.cpp"
	"Sqlite/Tables/SlateTable.cpp"
	"Sqlite/Tables/SlateContextTable.cpp"
	"Sqlite/Tables/TransactionsTable.cpp"
	"Sqlite/Tables/VersionTable.cpp"
	"Sqlite/Tables/AccountsTable.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WalletDB)
target_link_libraries(${TARGET_NAME} Core sqlite3)