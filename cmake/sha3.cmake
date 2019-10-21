set(TARGET_NAME sha3)

file(GLOB SRC
    "${PROJECT_SOURCE_DIR}/deps/sha3/sha3.cpp"
)


# sha3 Library
add_library(sha3 STATIC ${SRC})