set(TARGET_NAME TxPool)

file(GLOB SOURCE_CODE
    "TransactionBodyValidator.cpp"
	"TransactionPoolImpl.cpp"
	"TransactionValidator.cpp"
	"TransactionAggregator.cpp"
	"ValidTransactionFinder.cpp"
	"Pool.cpp"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_TX_POOL)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core PMMR)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core PMMR)