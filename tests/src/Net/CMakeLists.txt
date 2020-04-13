set(TARGET_NAME Net_Tests)

file(GLOB SOURCE_CODE
	"*.cpp"
	"tor/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
add_dependencies(${TARGET_NAME} Net fmt::fmt)
target_link_libraries(${TARGET_NAME} Net fmt::fmt)