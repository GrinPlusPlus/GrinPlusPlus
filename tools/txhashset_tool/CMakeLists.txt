set(TARGET_NAME txhashset_tool)

add_executable(${TARGET_NAME} "txhashset_tool.cpp")
target_link_libraries(${TARGET_NAME} PRIVATE Common Core Net P2P Tor BlockChain PMMR PoW API)