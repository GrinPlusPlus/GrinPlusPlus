set(TARGET_NAME Core_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
	"Models/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})

add_dependencies(${TARGET_NAME} Core)
target_link_libraries(${TARGET_NAME} Core)