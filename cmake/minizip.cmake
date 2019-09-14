set(TARGET_NAME Minizip)

file(GLOB MINIZIP_SRC
    "${PROJECT_SOURCE_DIR}/deps/zlib/contrib/minizip/ioapi.c"
    "${PROJECT_SOURCE_DIR}/deps/zlib/contrib/minizip/zip.c"
    "${PROJECT_SOURCE_DIR}/deps/zlib/contrib/minizip/unzip.c"
)

include_directories(
	${PROJECT_SOURCE_DIR}/deps/zlib/contrib
)

add_library(${TARGET_NAME} STATIC ${MINIZIP_SRC})

#target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)

#add_dependencies(${TARGET_NAME} Infrastructure Core Crypto)
#target_link_libraries(${TARGET_NAME})