set(TARGET_NAME Server)

file(GLOB SERVER_SRC
    "civetweb/src/CivetServer.cpp"
    "civetweb/src/civetweb.c"
	"Main.cpp"
	"Server.cpp"
	"NodeRestServer.cpp"
	"API/BlockAPI.cpp"
	"API/ChainAPI.cpp"
	"API/HeaderAPI.cpp"
	"API/PeersAPI.cpp"
	"API/ServerAPI.cpp"
	"API/TxHashSetAPI.cpp"
	"JSONFactory.cpp"
)

include_directories(civetweb/include)

#JsonCPP
hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)
set_target_properties(jsoncpp_lib_static PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

add_executable(${TARGET_NAME} ${SERVER_SRC})

add_dependencies(${TARGET_NAME} Infrastructure P2P BlockChain Config)
target_link_libraries(${TARGET_NAME} Infrastructure P2P BlockChain Config jsoncpp_lib_static)