set(TARGET_NAME Wallet)

# Wallet
file(GLOB Wallet_SRC
	"Wallet.cpp"
	"Sender.cpp"
	"SessionManager.cpp"
	"WalletServerImpl.cpp"
	"WalletUtil.cpp"
)

add_subdirectory(WalletDB)
add_subdirectory(Keychain)

add_library(${TARGET_NAME} SHARED ${Wallet_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB)