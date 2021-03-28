set(TARGET_NAME Wallet)

# Wallet
file(GLOB SOURCE_CODE
	"SlateBuilder/CoinSelection.cpp"
	"SlateBuilder/SendSlateBuilder.cpp"
	"SlateBuilder/ReceiveSlateBuilder.cpp"
	"SlateBuilder/FinalizeSlateBuilder.cpp"
	"Models/Slate/Slate.cpp"
	"CancelTx.cpp"
	"Wallet.cpp"
	"WalletImpl.cpp"
	"WalletRefresher.cpp"
	"SessionManager.cpp"
	"WalletManagerImpl.cpp"
	"WalletTxLoader.cpp"
	"OutputRestorer.cpp"
	"ForeignController.cpp"
)

add_subdirectory(WalletDB)
add_subdirectory(Keychain)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)
target_link_libraries(${TARGET_NAME} API Common Crypto Core Keychain WalletDB Tor)

if (UNIX AND NOT APPLE)
	target_link_libraries(${TARGET_NAME} uuid)
endif()