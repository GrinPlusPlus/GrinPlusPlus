set(TARGET_NAME Wallet_Tests)

file(GLOB SOURCE_CODE
	"Tests/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE} ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Config Keychain WalletDB Wallet)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Config Keychain WalletDB Wallet)