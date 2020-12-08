set(TARGET_NAME BlockChain)

file(GLOB SOURCE_CODE
    "BlockChainImpl.cpp"
    "BlockHydrator.cpp"
    "Chain.cpp"
    "ChainResyncer.cpp"
    "ChainState.cpp"
    "ChainStore.cpp"
    "CoinView.cpp"
    "CompactBlockFactory.cpp"
	"Processors/*.cpp"
	"Validators/*.cpp"
	"OrphanPool/OrphanPool.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_BLOCK_CHAIN)
target_link_libraries(${TARGET_NAME} Common Crypto Core Database PoW PMMR TxPool)