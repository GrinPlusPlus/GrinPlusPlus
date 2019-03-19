set(TARGET_NAME GrinNode)

file(GLOB SERVER_SRC
    "civetweb/src/CivetServer.cpp"
    "civetweb/src/civetweb.c"
	"Main.cpp"
	"JSONFactory.cpp"
	"ShutdownManager.cpp"
	"Node/NodeDaemon.cpp"
	"Node/NodeRestServer.cpp"
	"Node/API/BlockAPI.cpp"
	"Node/API/ChainAPI.cpp"
	"Node/API/HeaderAPI.cpp"
	"Node/API/PeersAPI.cpp"
	"Node/API/ServerAPI.cpp"
	"Node/API/TxHashSetAPI.cpp"
	"Node/API/Explorer/BlockInfoAPI.cpp"
	"Wallet/WalletDaemon.cpp"
	"Wallet/WalletRestServer.cpp"
	"Wallet/API/OwnerGetAPI.cpp"
	"Wallet/API/OwnerPostAPI.cpp"
)

include_directories(civetweb/include)

#JsonCPP
hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)
set_target_properties(jsoncpp_lib_static PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

add_executable(${TARGET_NAME} ${SERVER_SRC})

add_dependencies(${TARGET_NAME} Infrastructure P2P BlockChain Config Wallet)
target_link_libraries(${TARGET_NAME} Infrastructure P2P BlockChain Config jsoncpp_lib_static Wallet)