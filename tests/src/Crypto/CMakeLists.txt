set(TARGET_NAME Crypto_Tests)

file(GLOB SOURCE_CODE
	"*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_CRYPTO INCLUDE_TEST_MATH)
add_dependencies(${TARGET_NAME} Crypto fmt::fmt)
target_link_libraries(${TARGET_NAME} Crypto fmt::fmt)