set(TARGET_NAME API_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
    "Wallet/Foreign/*.cpp"
    "Wallet/Owner/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core BlockChain fmt::fmt Keychain Wallet civetweb::civetweb)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core BlockChain fmt::fmt Keychain Wallet civetweb::civetweb)