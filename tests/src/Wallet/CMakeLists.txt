set(TARGET_NAME Wallet_Tests)

file(GLOB SOURCE_CODE
	"*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)
target_link_libraries(${TARGET_NAME} Common Crypto Core BlockChain Keychain WalletDB Wallet TestUtil chachapoly)