set(TARGET_NAME Wallet)

# Wallet
file(GLOB Wallet_SRC
    "Keychain/KeyGenerator.cpp"
    "Keychain/SeedEncrypter.cpp"
	"Keychain/PublicKeyCalculator.cpp"
)

add_library(${TARGET_NAME} SHARED ${Wallet_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_Wallet)
add_dependencies(${TARGET_NAME} Infrastructure Crypto Core)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core)