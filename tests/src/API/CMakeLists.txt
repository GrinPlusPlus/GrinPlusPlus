set(TARGET_NAME API_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
    "Wallet/Foreign/*.cpp"
    "Wallet/Owner/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} Common Crypto Core BlockChain Keychain Wallet TestUtil)