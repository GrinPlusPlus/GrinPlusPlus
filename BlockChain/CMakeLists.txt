set(TARGET_NAME BlockChain)

file(GLOB BLOCK_CHAIN_SRC
    "*.cpp"
	"Processors/*.cpp"
	"Validators/*.cpp"
	"OrphanPool/OrphanPool.cpp"
)

add_library(${TARGET_NAME} SHARED ${BLOCK_CHAIN_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_BLOCK_CHAIN)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Database PoW PMMR TxPool)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Database PoW PMMR TxPool)