set(TARGET_NAME GrinNode_Tests)

file(GLOB SOURCE_CODE
	"*.cpp"
	"Wallet/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
add_dependencies(${TARGET_NAME}  Infrastructure P2P BlockChain Wallet PoW)
target_link_libraries(${TARGET_NAME}  Infrastructure P2P BlockChain Wallet PoW)