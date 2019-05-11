set(TARGET_NAME WalletDB)

# Wallet
file(GLOB WalletDB_SRC
    "WalletDBImpl.cpp"
)

hunter_add_package(rocksdb)
find_package(RocksDB CONFIG REQUIRED)
set_target_properties(RocksDB::rocksdb PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

find_package(gflags CONFIG REQUIRED)

add_library(${TARGET_NAME} SHARED ${WalletDB_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WalletDB)

target_link_libraries(${TARGET_NAME} Core RocksDB::rocksdb gflags)