set(TARGET_NAME Keychain)

# Wallet
file(GLOB Keychain_SRC
	"KeyChain.cpp"
    "KeyGenerator.cpp"
    "SeedEncrypter.cpp"
	"PublicKeyCalculator.cpp"
	"Mnemonic.cpp"
)

add_library(${TARGET_NAME} STATIC ${Keychain_SRC})
add_dependencies(${TARGET_NAME} Infrastructure Crypto Core)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core "-framework CoreFoundation")