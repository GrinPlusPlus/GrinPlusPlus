set(TARGET_NAME Crypto)

add_definitions(-DHAVE_CONFIG_H -DHAVE_SCRYPT_CONFIG_H)
include_directories(${PROJECT_SOURCE_DIR}/Crypto/secp256k1-zkp)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W2 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2 /WX")
    endif()
endif()

file(GLOB CRYPTO_SRC
	"AES256.cpp"
    "Blake2b.cpp"
	"Crypto.cpp"
	"Secp256k1Wrapper.cpp"
	"RandomNumberGenerator.cpp"
	"scrypt/crypto_scrypt-ref.cpp"
	"scrypt/sha256.cpp"
	"secp256k1-zkp/src/secp256k1.c"
)

add_library(Crypto SHARED ${CRYPTO_SRC})

target_compile_definitions(${TARGET_NAME} PRIVATE MW_CRYPTO)

add_dependencies(${TARGET_NAME} Infrastructure)
target_link_libraries(${TARGET_NAME} Infrastructure)
