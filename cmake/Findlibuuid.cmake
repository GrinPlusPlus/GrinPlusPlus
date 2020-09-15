add_library(uuid INTERFACE)

find_path(LIBUUID_INCLUDE_DIR uuid.h PATH_SUFFIXES uuid)
find_library(LIBUUID_LIBRARY libuuid.a)
target_link_libraries(uuid INTERFACE ${LIBUUID_LIBRARY})
target_include_directories(uuid SYSTEM INTERFACE ${LIBUUID_INCLUDE_DIR})