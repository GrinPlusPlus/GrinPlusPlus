set(TARGET_NAME Database_Tests)

file(GLOB SOURCE_CODE
	"*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)
add_dependencies(${TARGET_NAME} Database fmt TestUtil)
target_link_libraries(${TARGET_NAME} Database fmt TestUtil)