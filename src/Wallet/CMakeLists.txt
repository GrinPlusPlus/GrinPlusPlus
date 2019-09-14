set(TARGET_NAME Wallet)

# Wallet
file(GLOB SOURCE_CODE
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

if (LINUX)
	set(OS_LIBS uuid)
else()
	set(OS_LIBS "")
endif()

add_subdirectory(WalletDB)
add_subdirectory(Keychain)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_WALLET)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core Keychain WalletDB ${OS_LIBS})