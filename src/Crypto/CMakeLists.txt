set(TARGET_NAME Crypto)

add_definitions(-DHAVE_CONFIG_H -DHAVE_SCRYPT_CONFIG_H -DSECP256K1_BUILD)
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/secp256k1-zkp
	${CMAKE_CURRENT_SOURCE_DIR}/secp256k1-zkp/src
)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
			string(REGEX REPLACE "/W[0-4]" "/W2 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2 /WX")
    endif()
endif()

if(WIN32)
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libsecp256k1-config.h ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1-zkp/src/libsecp256k1-config.h COPYONLY)
else()
		include_directories(${GOBJECT_INCLUDE_DIR})
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libsecp256k1-config-mac.h ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1-zkp/src/libsecp256k1-config.h COPYONLY)
endif()

file(GLOB SOURCE_CODE
	"aes.cpp"
	"AggSig.cpp"
	"ctaes/ctaes.c"
	"Blake2b.cpp"
	"Bulletproofs.cpp"
	"Crypto.cpp"
	"ECDH.cpp"
	"sha256.cpp"
	"sha512.cpp"
	"hmac_sha256.cpp"
	"hmac_sha512.cpp"
	"ripemd160.cpp"
	"Pedersen.cpp"
	"PublicKeys.cpp"
	"RandomNumberGenerator.cpp"
	"scrypt/crypto_scrypt-ref.cpp"
	"scrypt/sha256.cpp"
	"secp256k1-zkp/src/secp256k1.c"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_CRYPTO)

add_dependencies(${TARGET_NAME} Infrastructure)
target_link_libraries(${TARGET_NAME} PUBLIC Infrastructure)