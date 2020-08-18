set(TARGET_NAME bitcoin)

file(GLOB SOURCE_CODE
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/bech32.cpp"
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/ctaes.c"
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/aes.cpp"
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/hmac_sha256.cpp"
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/hmac_sha512.cpp"
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/ripemd160.cpp"
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/sha256.cpp"
    "${PROJECT_SOURCE_DIR}/deps/bitcoin/src/sha512.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/deps/bitcoin/include)