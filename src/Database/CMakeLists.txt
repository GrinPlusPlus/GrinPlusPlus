set(TARGET_NAME Database)

file(GLOB SOURCE_CODE
	"*.h"
	"*.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)
target_link_libraries(${TARGET_NAME} PUBLIC Common Core Crypto RocksDB::rocksdb)