set(TARGET_NAME BlockChain_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} Common Crypto Core BlockChain Keychain TestUtil)