set(TARGET_NAME P2P)

file(GLOB SOURCE_CODE
    "*.cpp"
	"Seed/*.cpp"
	"Sync/*.cpp"
	"Pipeline/*.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_P2P)
target_link_libraries(${TARGET_NAME} Common Core Crypto Database Net)
