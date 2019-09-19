set(TARGET_NAME scrypt)

file(GLOB SOURCE_CODE
    "${PROJECT_SOURCE_DIR}/deps/scrypt/crypto_scrypt-ref.cpp"
	"${PROJECT_SOURCE_DIR}/deps/scrypt/sha256.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE HAVE_SCRYPT_CONFIG_H)