set(TARGET_NAME Core_Tests)

file(GLOB CORE_TESTS_SRC
    "*.h"
    "*.cpp"
	"Models/*.h"
	"Models/*.cpp"
)

add_executable(${TARGET_NAME} ${CORE_TESTS_SRC})

add_dependencies(${TARGET_NAME} Core)
target_link_libraries(${TARGET_NAME} Core)