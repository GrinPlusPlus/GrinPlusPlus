set(TARGET_NAME Database)

file(GLOB DATABASE_SRC
	"*.h"
	"*.cpp"
)

hunter_add_package(rocksdb)
find_package(RocksDB CONFIG REQUIRED)
set_target_properties(RocksDB::rocksdb PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

if (WIN32)
    set(OS_LIBS "")
else(WIN32)
	hunter_add_package(gflags)
	find_package(gflags CONFIG REQUIRED)

	set(OS_LIBS gflags)
endif(WIN32)

add_library(${TARGET_NAME} SHARED ${DATABASE_SRC})

target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto)
target_link_libraries(${TARGET_NAME} PUBLIC Infrastructure Core Crypto RocksDB::rocksdb ${OS_LIBS})