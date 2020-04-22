set(TARGET_NAME Core_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
	"Models/*.cpp"
	"File/*.cpp"
)

remove_definitions(-DNOMINMAX)

if (LINUX)
	set(OS_LIBS uuid)
else()
	set(OS_LIBS "")
endif()

add_executable(${TARGET_NAME} ${SOURCE_CODE})

add_dependencies(${TARGET_NAME} Core TestUtil)
target_link_libraries(${TARGET_NAME} Core TestUtil Wallet ${OS_LIBS})