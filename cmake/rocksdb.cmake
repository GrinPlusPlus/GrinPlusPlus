
set(WITH_WINDOWS_UTF8_FILENAMES TRUE)
set(WITH_TESTS FALSE)
include_directories(${PROJECT_SOURCE_DIR}/deps/rocksdb)
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/rocksdb)

set(ROCKS_DB_LIB rocksdb${ARTIFACT_SUFFIX})
