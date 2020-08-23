set(TARGET_NAME Crypto)

file(GLOB SOURCE_CODE
	"AES256.cpp"
	"Age.cpp"
	"AggSig.cpp"
	"Bulletproofs.cpp"
	"Crypto.cpp"
	"CSPRNG.cpp"
	"ED25519.cpp"
	"Hasher.cpp"
	"KDF.cpp"
	"Pedersen.cpp"
	"PublicKeys.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_CRYPTO NOMINMAX)
target_link_libraries(${TARGET_NAME} PUBLIC bitcoin scrypt secp256k1-zkp::secp256k1-zkp unofficial-sodium::sodium chachapoly rfc6234 Common)