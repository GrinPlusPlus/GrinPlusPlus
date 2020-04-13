set(TARGET_NAME Wallet_Tests)

file(GLOB SOURCE_CODE
	"*.cpp"
)

remove_definitions(-DNOMINMAX)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core BlockChain Keychain WalletDB Wallet civetweb::civetweb)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core BlockChain Keychain WalletDB Wallet civetweb::civetweb)