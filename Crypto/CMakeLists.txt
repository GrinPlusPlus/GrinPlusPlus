set(TARGET_NAME Crypto)
set(TEST_TARGET_NAME Crypto_Tests)

add_definitions(-DHAVE_CONFIG_H -DHAVE_SCRYPT_CONFIG_H -DSECP256K1_BUILD)
include_directories(
	${PROJECT_SOURCE_DIR}/Crypto/secp256k1-zkp
	${PROJECT_SOURCE_DIR}/Crypto/secp256k1-zkp/src
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

file(GLOB CRYPTO_SRC
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

add_library(${TARGET_NAME} SHARED ${CRYPTO_SRC})

target_compile_definitions(${TARGET_NAME} PRIVATE MW_CRYPTO)

add_dependencies(${TARGET_NAME} Infrastructure)
target_link_libraries(${TARGET_NAME} PUBLIC Infrastructure)

# Tests
file(GLOB CRYPTO_TESTS_SRC
	"Tests/*.cpp"
)

if (WIN32)
    set(OS_SPECIFIC_LIBS "")
else(WIN32)
    find_library(OS_SPECIFIC_LIBS pthread GMP)
endif(WIN32)

add_executable(${TEST_TARGET_NAME} ${CRYPTO_SRC} ${CRYPTO_TESTS_SRC})
target_compile_definitions(${TEST_TARGET_NAME} PRIVATE MW_CRYPTO INCLUDE_TEST_MATH)
add_dependencies(${TEST_TARGET_NAME} Infrastructure)
target_link_libraries(${TEST_TARGET_NAME} ${OS_SPECIFIC_LIBS} Infrastructure)
