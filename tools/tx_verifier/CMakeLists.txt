set(TARGET_NAME tx_verifier)

add_executable(${TARGET_NAME} "tx_verifier.cpp")
target_link_libraries(${TARGET_NAME} PRIVATE Common Core Net P2P Tor BlockChain Wallet PoW API)