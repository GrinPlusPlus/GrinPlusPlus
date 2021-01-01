set(TARGET_NAME slate_tool)

add_executable(${TARGET_NAME} "slate_tool.cpp")
target_link_libraries(${TARGET_NAME} PRIVATE Common Core Net P2P Tor BlockChain Wallet PoW API)