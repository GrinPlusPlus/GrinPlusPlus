set(TARGET_NAME PMMR)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W2 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2 /WX")
    endif()
endif()

include_directories(
	${PROJECT_SOURCE_DIR}/deps/zlib/contrib
)

# PMMR
file(GLOB SOURCE_CODE
    "HeaderMMRImpl.cpp"
    "KernelMMR.cpp"
    "OutputPMMR.cpp"
    "RangeProofPMMR.cpp"
    "TxHashSetImpl.cpp"
    "TxHashSetManager.cpp"
    "TxHashSetValidator.cpp"
    "Common/*.cpp"
    "Common/CRoaring/*.c"
    "Zip/*.cpp"
)


if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_PMMR)
add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Minizip jsoncpp)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core zlib Minizip jsoncpp)

if(GRINPP_TESTS)
	set(TEST_TARGET_NAME PMMR_Tests)

	file(GLOB PMMR_TESTS_SRC
		"Tests/*.cpp"
	)

	add_executable(${TEST_TARGET_NAME} ${PMMR_SRC} ${PMMR_TESTS_SRC})
	target_compile_definitions(${TEST_TARGET_NAME} PRIVATE MW_PMMR)
	add_dependencies(${TEST_TARGET_NAME} Infrastructure Crypto Core Minizip jsoncpp)
	target_link_libraries(${TEST_TARGET_NAME} Infrastructure Crypto Core zlib Minizip jsoncpp)
endif(GRINPP_TESTS)