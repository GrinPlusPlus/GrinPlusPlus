set(TARGET_NAME Net_Tests)

file(GLOB SOURCE_CODE
	"*.cpp"
	"tor/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} Net TestUtil Database TxPool BlockChain Wallet)