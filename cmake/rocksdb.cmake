
set(WITH_WINDOWS_UTF8_FILENAMES TRUE)
set(WITH_TESTS FALSE)
if (UNIX)
    set(USE_RTTI 1)
endif ()

include_directories(${PROJECT_SOURCE_DIR}/deps/rocksdb-6.7.3)
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/rocksdb-6.7.3)

set(ROCKS_DB_LIB rocksdb${ARTIFACT_SUFFIX})
