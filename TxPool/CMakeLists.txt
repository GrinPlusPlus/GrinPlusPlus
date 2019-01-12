set(TARGET_NAME TxPool)
set(TEST_TARGET_NAME TxPool_Tests)

file(GLOB TX_POOL_SRC
    "TransactionBodyValidator.cpp"
	"TransactionPoolImpl.cpp"
	"TransactionValidator.cpp"
	"TransactionAggregator.cpp"
	"ValidTransactionFinder.cpp"
	"Pool.cpp"
)

add_library(${TARGET_NAME} SHARED ${TX_POOL_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_TX_POOL)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core PMMR)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core PMMR)

# Tests
file(GLOB TX_POOL_TESTS_SRC
	"Tests/*.cpp"
)

#add_executable(${TEST_TARGET_NAME} ${TX_POOL_SRC} ${TX_POOL_TESTS_SRC})
#target_compile_definitions(${TEST_TARGET_NAME} PRIVATE MW_TX_POOL)

#add_dependencies(${TEST_TARGET_NAME} Infrastructure Crypto Core PMMR)
#target_link_libraries(${TEST_TARGET_NAME} Infrastructure Crypto Core PMMR)