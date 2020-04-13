set(TARGET_NAME BlockChain_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core BlockChain fmt::fmt Keychain)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core BlockChain fmt::fmt Keychain)