set(TARGET_NAME P2P)

file(GLOB P2P_SRC
    "*.cpp"
	"Seed/*.cpp"
	"Sync/*.cpp"
)

add_library(${TARGET_NAME} SHARED ${P2P_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_P2P)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto Database)
target_link_libraries(${TARGET_NAME} Infrastructure Core Crypto Database)
