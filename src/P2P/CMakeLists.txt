set(TARGET_NAME P2P)

file(GLOB SOURCE_CODE
    "*.cpp"
	"Seed/*.cpp"
	"Sync/*.cpp"
	"Pipeline/*.cpp"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
else(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_P2P)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto Database Net)
target_link_libraries(${TARGET_NAME} Infrastructure Core Crypto Database Net)
