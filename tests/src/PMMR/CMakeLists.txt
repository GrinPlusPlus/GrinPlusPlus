set(TARGET_NAME PMMR_Tests)

# PMMR
file(GLOB SOURCE_CODE
    #"Test_ValidateTxHashSet.cpp"
	#"TestMain.cpp"
	"*.cpp"
	"LeafSet/*.cpp"
)

add_executable(${TARGET_NAME} ${PMMR_SRC}  ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_PMMR)
target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_dependencies(${TARGET_NAME} Infrastructure BlockChain TxPool Database Crypto Core minizip::minizip jsoncpp_lib PMMR)
target_link_libraries(${TARGET_NAME} Infrastructure BlockChain TxPool Database Crypto Core ZLIB::ZLIB minizip::minizip jsoncpp_lib PMMR)