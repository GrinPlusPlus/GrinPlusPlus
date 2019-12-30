set(REPROC++ ON)
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/reproc)
include_directories(
    ${PROJECT_SOURCE_DIR}/deps/reproc/reproc++/include
    ${CMAKE_CURRENT_BINARY_DIR}/deps/reproc/reproc++/include
)