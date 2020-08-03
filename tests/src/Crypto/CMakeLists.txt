set(TARGET_NAME Crypto_Tests)

file(GLOB SOURCE_CODE
	"Test_AddCommitments.cpp"
	"Test_AggSig.cpp"
	"Test_ChaChaPoly.cpp"
	"Test_ED25519.cpp"
	"TestMain.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_CRYPTO INCLUDE_TEST_MATH)
target_link_libraries(${TARGET_NAME} Crypto TestUtil)