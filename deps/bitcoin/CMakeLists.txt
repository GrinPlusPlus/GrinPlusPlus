set(TARGET_NAME bitcoin)

add_library(${TARGET_NAME} STATIC
    "src/bech32.cpp"
    "src/ctaes.c"
    "src/aes.cpp"
    "src/ripemd160.cpp"
    "src/sha256.cpp"
    "src/sha512.cpp"
)
target_include_directories(${TARGET_NAME} PUBLIC "include/")