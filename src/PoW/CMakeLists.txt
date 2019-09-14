set(TARGET_NAME PoW)

file(GLOB SOURCE_CODE
    "*.cpp"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_POW)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto Cuckoo uint128)
target_link_libraries(${TARGET_NAME} Infrastructure Core Crypto Cuckoo uint128)