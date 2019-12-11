set(TARGET_NAME PMMR_Tests)

# PMMR
file(GLOB SOURCE_CODE
    "Test_ValidateTxHashSet.cpp"
	"TestMain.cpp"
)

add_executable(${TARGET_NAME} ${PMMR_SRC}  ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_PMMR)
add_dependencies(${TARGET_NAME} Infrastructure BlockChain TxPool Database Crypto Core Minizip jsoncpp PMMR)
target_link_libraries(${TARGET_NAME} Infrastructure BlockChain TxPool Database Crypto Core zlib Minizip jsoncpp PMMR)