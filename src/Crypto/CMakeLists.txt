set(TARGET_NAME Crypto)

file(GLOB SOURCE_CODE
	"Age.cpp"
	"AggSig.cpp"
	"Bulletproofs.cpp"
	"Crypto.cpp"
	"Pedersen.cpp"
	"PublicKeys.cpp"
	"RandomNumberGenerator.cpp"
	"ThirdParty/Blake2b.cpp"
	"ThirdParty/sha256.cpp"
	"ThirdParty/sha512.cpp"
	"ThirdParty/hmac_sha256.cpp"
	"ThirdParty/hmac_sha512.cpp"
	"ThirdParty/ripemd160.cpp"
	"ThirdParty/aes.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_CRYPTO)
target_link_libraries(${TARGET_NAME} PUBLIC ctaes scrypt secp256k1-zkp::secp256k1-zkp unofficial-sodium::sodium chachapoly rfc6234 Common)