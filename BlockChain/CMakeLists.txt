set(TARGET_NAME BlockChain)

file(GLOB BLOCK_CHAIN_SRC
    "*.cpp"
	"uint128/*.cpp"
	"Processors/*.cpp"
	"Validators/*.cpp"
)

add_library(${TARGET_NAME} SHARED ${BLOCK_CHAIN_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_BLOCK_CHAIN)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Database PMMR)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Database PMMR)