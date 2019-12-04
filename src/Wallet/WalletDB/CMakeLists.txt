set(TARGET_NAME WalletDB)

# Wallet

file(GLOB SOURCE_CODE
    "WalletDBImpl.cpp"
    "WalletSqlite.cpp"
	"WalletEncryptionUtil.cpp"
	"Sqlite/*.cpp"
	"Sqlite/Tables/VersionTable.cpp"
	"Sqlite/Tables/OutputsTable.cpp"
	"Sqlite/Tables/TransactionsTable.cpp"
	"Sqlite/Tables/MetadataTable.cpp"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
else(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_WalletDB)

add_dependencies(${TARGET_NAME} sqlite3)
target_link_libraries(${TARGET_NAME} Core sqlite3)