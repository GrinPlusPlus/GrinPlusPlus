set(TARGET_NAME WalletDB)

# Wallet
file(GLOB WalletDB_SRC
    "WalletDBImpl.cpp"
    "WalletRocksDB.cpp"
    "WalletSqlite.cpp"
)

if (WIN32)
    set(OS_LIBS "")
else(WIN32)
	hunter_add_package(gflags)
	find_package(gflags CONFIG REQUIRED)
	set(OS_LIBS gflags)
endif(WIN32)

add_library(${TARGET_NAME} SHARED ${WalletDB_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WalletDB)

add_dependencies(${TARGET_NAME} sqlite3)
target_link_libraries(${TARGET_NAME} Core rocksdb${ARTIFACT_SUFFIX} sqlite3 ${OS_LIBS})