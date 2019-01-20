set(TARGET_NAME Database)

file(GLOB DATABASE_SRC
	"*.h"
	"*.cpp"
)

hunter_add_package(rocksdb)
find_package(RocksDB CONFIG REQUIRED)
set_target_properties(RocksDB::rocksdb PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

add_library(${TARGET_NAME} SHARED ${DATABASE_SRC})

target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto)
target_link_libraries(${TARGET_NAME} PUBLIC Infrastructure Core Crypto RocksDB::rocksdb)