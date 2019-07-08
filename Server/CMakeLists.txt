set(TARGET_NAME GrinNode)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W0 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0 /WX")
    endif()
endif()

file(GLOB SERVER_SRC
    "civetweb/src/CivetServer.cpp"
    "civetweb/src/civetweb.c"
	"thirdparty/i2psam/i2psam.cpp"
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

add_executable(${TARGET_NAME} ${SERVER_SRC})

add_dependencies(${TARGET_NAME} Infrastructure P2P BlockChain Config Wallet)
target_compile_definitions(${TARGET_NAME} PRIVATE ssize_t=SSIZE_T)
target_link_libraries(${TARGET_NAME} Infrastructure P2P BlockChain Config Wallet)