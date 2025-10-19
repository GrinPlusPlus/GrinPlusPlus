set(TARGET_NAME GrinNode)

add_executable(${TARGET_NAME} 
	"Main.cpp"
	"Node/Node.cpp"
	"Node/NodeRPCServer.cpp"
	"Node/API/BlockAPI.cpp"
	"Node/API/ChainAPI.cpp"
	"Node/API/HeaderAPI.cpp"
	"Node/API/PeersAPI.cpp"
	"Node/API/ServerAPI.cpp"
	"Node/API/TxHashSetAPI.cpp"
	"Wallet/WalletDaemon.cpp"
)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${TARGET_NAME} Common P2P BlockChain Wallet PoW API)