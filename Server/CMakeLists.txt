set(TARGET_NAME Server)

file(GLOB SERVER_SRC
    "civetweb/src/CivetServer.cpp"
    "civetweb/src/civetweb.c"
)

include_directories(civetweb/include)

#JsonCPP
hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)

add_executable(${TARGET_NAME} Main.cpp Server.cpp RestServer.cpp BlockAPI.cpp ${SERVER_SRC})

add_dependencies(${TARGET_NAME} Infrastructure P2P BlockChain Config)
target_link_libraries(${TARGET_NAME} Infrastructure P2P BlockChain Config jsoncpp_lib_static)