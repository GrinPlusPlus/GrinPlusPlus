set(TARGET_NAME libgrin.so)

file(GLOB SOURCE_CODE
	"Main.cpp"
	"JSONFactory.cpp"
	"ShutdownManager.cpp"
	"Node/Node.cpp"
	"Node/NodeRestServer.cpp"
	"Node/API/BlockAPI.cpp"
	"Node/API/ChainAPI.cpp"
	"Node/API/HeaderAPI.cpp"
	"Node/API/PeersAPI.cpp"
	"Node/API/ServerAPI.cpp"
	"Node/API/TxHashSetAPI.cpp"
	"Wallet/WalletDaemon.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE ssize_t=SSIZE_T)
target_link_libraries(${TARGET_NAME} Common P2P BlockChain Wallet PoW API)