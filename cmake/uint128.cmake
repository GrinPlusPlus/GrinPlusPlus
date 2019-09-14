set(TARGET_NAME uint128)

file(GLOB SOURCE_CODE
    "${PROJECT_SOURCE_DIR}/deps/uint128/*.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})