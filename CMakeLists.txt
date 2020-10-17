cmake_minimum_required(VERSION 3.8)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

SET(CMAKE_BUILD_TYPE Release)

if(APPLE)
    set(CMAKE_INSTALL_NAME_DIR "@executable_path")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

project(GRIN_PLUS_PLUS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "[cC][lL][aA][nN][gG]") #Case insensitive match
    message(FATAL_ERROR "Clang not supported")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -stdlib=libc++ -fno-aligned-allocation")
endif()

if(MSVC)
    message("Setting flags for Visual Studio C++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")

    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    endif()

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELWITHDEBINFO
    )

    add_definitions(-DWIN32_LEAN_AND_MEAN)
foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
    message("MSVC flags: ${CompilerFlag}:${${CompilerFlag}}")
endforeach()

    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /INCREMENTAL:NO")
    SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /OPT:REF /INCREMENTAL:NO")
    SET(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /OPT:REF /INCREMENTAL:NO")
else ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wno-deprecated-copy -Wno-pessimizing-move")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(WITH_GFLAGS OFF)
#add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)


include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/grinpp_macros.cmake)

include(CheckFunctionExists)
set(CMAKE_REQUIRED_INCLUDES string.h)
CHECK_FUNCTION_EXISTS(memset_s HAS_MEMSET_S)
if (HAS_MEMSET_S)
	add_definitions(-DHAS_MEMSET_S)
endif ()

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/deps
    ${PROJECT_SOURCE_DIR}/deps/mio/single_include
    ${PROJECT_SOURCE_DIR}/deps/stduuid/include
    ${PROJECT_SOURCE_DIR}/deps/ghc
    ${PROJECT_SOURCE_DIR}/deps/jsoncpp
    ${PROJECT_SOURCE_DIR}/deps/Catch2
    ${PROJECT_SOURCE_DIR}/deps/cppcodec
    ${PROJECT_SOURCE_DIR}/deps/scheduler/include
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# Dependencies
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_definitions(-DUNICODE)

find_package(uuid REQUIRED)
find_package(ZLIB REQUIRED)
find_package(RocksDB CONFIG REQUIRED)
find_package(civetweb CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(roaring CONFIG REQUIRED)
find_package(minizip CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(secp256k1-zkp CONFIG REQUIRED)
find_package(mio CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

include(sqlite3)
include(jsoncpp)
include(uint128)
include(scrypt)
include(sha3)
include(reproc)
include(bitcoin)
add_subdirectory(deps/cppcodec)
add_subdirectory(deps/chachapoly)
add_subdirectory(deps/rfc6234)

add_subdirectory(src)

option(GRINPP_TESTS "Build tests" true)
if(GRINPP_TESTS)
    add_subdirectory(tests)
endif()

option(GRINPP_TOOLS "Build tools" true)
if(GRINPP_TOOLS)
    add_subdirectory(tools)
endif()

target_link_libraries(${TARGET_NAME} stdc++fs)
