
set(WITH_TESTS FALSE)
include_directories(${PROJECT_SOURCE_DIR}/deps/rocksdb)
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/rocksdb)

if(APPLE)
	set(ROCKS_DB_LIB rocksdb-shared${ARTIFACT_SUFFIX})
else()
	set(ROCKS_DB_LIB rocksdb${ARTIFACT_SUFFIX})
endif()