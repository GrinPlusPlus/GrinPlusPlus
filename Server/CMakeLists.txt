set(TARGET_NAME Server)

add_executable(${TARGET_NAME} Main.cpp Server.cpp)

add_dependencies(${TARGET_NAME} Infrastructure P2P BlockChain Config)
target_link_libraries(${TARGET_NAME} Infrastructure P2P BlockChain Config)