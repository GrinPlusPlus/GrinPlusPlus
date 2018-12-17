set(TARGET_NAME PMMR_Tests)

file(GLOB PMMR_TESTS_SRC
    "*.cpp"
	"../*.cpp"
	"../Common/*.cpp"
	"../Common/CRoaring/*.c"
	"../Zip/*.cpp"
)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W2 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2 /WX")
    endif()
endif()

hunter_add_package(minizip)
hunter_add_package(ZLIB)

add_executable(${TARGET_NAME} ${PMMR_TESTS_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_PMMR)

find_package(minizip CONFIG REQUIRED)
find_package(ZLIB CONFIG REQUIRED)

add_dependencies(${TARGET_NAME} Infrastructure Crypto Core)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core minizip::minizip ZLIB::zlib)