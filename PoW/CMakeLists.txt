set(TARGET_NAME PoW)

file(GLOB POW_SRC
    "*.cpp"
    "uint128/*.cpp"
)

add_subdirectory(cuckoo)

add_library(${TARGET_NAME} SHARED ${POW_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_POW)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto Cuckoo)
target_link_libraries(${TARGET_NAME} Infrastructure Core Crypto Cuckoo)