add_library(uuid INTERFACE)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    find_library(LIBUUID uuid "${CMAKE_CURRENT_SOURCE_DIR}/packages/libuuid_arm64-linux/debug/lib/")
else()
    find_library(LIBUUID uuid "${CMAKE_CURRENT_SOURCE_DIR}/packages/libuuid_arm64-linux/lib/")
endif()

target_link_libraries(uuid INTERFACE "${LIBUUID}")
target_include_directories(uuid SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/libuuid_arm64-linux/include")