set(TARGET_NAME bitcoin)

file(GLOB SOURCE_CODE
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/bech32.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/deps/bitcoin/include)