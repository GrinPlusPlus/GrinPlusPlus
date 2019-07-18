set(TARGET_NAME Wallet)
set(TEST_TARGET_NAME Wallet_Tests)

# Wallet
file(GLOB Wallet_SRC
	"SlateBuilder/CoinSelection.cpp"
	"SlateBuilder/SendSlateBuilder.cpp"
	"SlateBuilder/ReceiveSlateBuilder.cpp"
	"SlateBuilder/FinalizeSlateBuilder.cpp"
	"Wallet.cpp"
	"WalletRefresher.cpp"
	"SessionManager.cpp"
	"WalletManagerImpl.cpp"
	"OutputRestorer.cpp"
)

remove_definitions(-DNOMINMAX)

add_subdirectory(WalletDB)
add_subdirectory(Keychain)

add_library(${TARGET_NAME} SHARED ${Wallet_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB uuid)

# Tests
file(GLOB Wallet_Tests_SRC
	"Tests/*.cpp"
)

add_executable(${TEST_TARGET_NAME} ${Wallet_SRC} ${Wallet_Tests_SRC})
target_compile_definitions(${TEST_TARGET_NAME} PRIVATE MW_WALLET)

add_dependencies(${TEST_TARGET_NAME} Infrastructure Crypto Core Config Keychain WalletDB)
target_link_libraries(${TEST_TARGET_NAME} Infrastructure Crypto Core Config Keychain WalletDB uuid)