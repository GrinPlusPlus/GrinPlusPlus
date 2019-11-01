set(TARGET_NAME BlockChain)

file(GLOB SOURCE_CODE
    "*.cpp"
	"Processors/*.cpp"
	"Validators/*.cpp"
	"OrphanPool/OrphanPool.cpp"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
else(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_BLOCK_CHAIN)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Database PoW PMMR TxPool)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Database PoW PMMR TxPool)