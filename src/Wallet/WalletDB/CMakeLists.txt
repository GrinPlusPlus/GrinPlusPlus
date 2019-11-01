set(TARGET_NAME WalletDB)

# Wallet

if(LINUX)
    set(ROCKS_SRC "")
    set(OS_LIBS "")
else(LINUX)
    set(ROCKS_SRC "WalletRocksDB.cpp")
    set(OS_LIBS ${ROCKS_DB_LIB})
endif(LINUX)

file(GLOB SOURCE_CODE
    "WalletDBImpl.cpp"
    "WalletSqlite.cpp"
	"WalletEncryptionUtil.cpp"
	"Tables/VersionTable.cpp"
	"Tables/OutputsTable.cpp"
	"Tables/TransactionsTable.cpp"
	"Tables/MetadataTable.cpp"
    ${ROCKS_SRC}
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
else(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_WalletDB)

add_dependencies(${TARGET_NAME} sqlite3)
target_link_libraries(${TARGET_NAME} Core sqlite3 ${OS_LIBS})