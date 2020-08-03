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
	"WalletRefresher.cpp"
	"SessionManager.cpp"
	"WalletManagerImpl.cpp"
	"WalletTxLoader.cpp"
	"OutputRestorer.cpp"
	"ForeignController.cpp"
)

remove_definitions(-DNOMINMAX)

if (UNIX AND NOT APPLE)
	set(OS_LIBS uuid)
else()
	set(OS_LIBS "")
endif()

add_subdirectory(WalletDB)
add_subdirectory(Keychain)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
else(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB Tor)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB Tor ${OS_LIBS})