set(TARGET_NAME TxPool)

file(GLOB SOURCE_CODE
    "TransactionBodyValidator.cpp"
	"TransactionPoolImpl.cpp"
	"TransactionValidator.cpp"
	"TransactionAggregator.cpp"
	"ValidTransactionFinder.cpp"
	"Pool.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_TX_POOL)
target_link_libraries(${TARGET_NAME} Common Crypto Core PMMR)