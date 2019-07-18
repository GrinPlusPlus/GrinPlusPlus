set(TARGET_NAME WalletDB)

# Wallet

if(LINUX)
    set(ROCKS_SRC "")
    set(OS_LIBS "")
else(LINUX)
    set(ROCKS_SRC "WalletRocksDB.cpp")
    set(OS_LIBS ${ROCKS_DB_LIB})
endif(LINUX)

file(GLOB WalletDB_SRC
    "WalletDBImpl.cpp"
    "WalletSqlite.cpp"
    ${ROCKS_SRC}
)

add_library(${TARGET_NAME} SHARED ${WalletDB_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WalletDB)

add_dependencies(${TARGET_NAME} sqlite3)
target_link_libraries(${TARGET_NAME} Core sqlite3 ${OS_LIBS})