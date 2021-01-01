set(TARGET_NAME tx_verifier)

add_executable(${TARGET_NAME} "tx_verifier.cpp")
target_link_libraries(${TARGET_NAME} PRIVATE Common P2P Core Crypto BlockChain Net Tor PoW)