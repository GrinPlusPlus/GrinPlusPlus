set(TARGET_NAME Cuckoo)

file(GLOB CUCKOO_SRC
    "crypto/blake2b-ref.c"
	"crypto/siphash.cpp"
)

add_library(${TARGET_NAME} STATIC ${CUCKOO_SRC})
target_link_libraries(${TARGET_NAME})