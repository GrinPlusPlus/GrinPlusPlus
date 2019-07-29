cmake_minimum_required(VERSION 3.1)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

SET(CMAKE_BUILD_TYPE RelWithDebInfo) 

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if(APPLE)
	set(CMAKE_INSTALL_NAME_DIR "@executable_path")
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
elseif(LINUX)
	set(CMAKE_INSTALL_RPATH "$ORIGIN")
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

#set(BUILD_STATIC_LIBS ON)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 6)
set(VERSION_REVISION 2)

project(GRIN_PLUS_PLUS VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

message("Grin++ Version: ${PROJECT_VERSION}")

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")

    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # unreferenced formal parameter
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4324") # '__blake2b_state': structure was padded due to alignment specifier

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2")

    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /INCREMENTAL:NO")
    SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /OPT:REF /INCREMENTAL:NO")
    SET(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /OPT:REF /INCREMENTAL:NO")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "[cC][lL][aA][nN][gG]") #Case insensitive match
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -stdlib=libc++ -fno-aligned-allocation")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-copy -Wno-pessimizing-move -Wno-maybe-uninitialized")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(WITH_GFLAGS OFF)
add_definitions(-DNDEBUG)
add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_definitions(-DNOMINMAX)
add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
add_definitions(-DASIO_STANDALONE)

#if(APPLE)
#	add_definitions(-DGRINPP_USER_AGENT="Grin++ ${PROJECT_VERSION}-MAC")
#elseif(LINUX)
#	add_definitions(-DGRINPP_USER_AGENT="Grin++ ${PROJECT_VERSION}-LINUX")
#else()
	add_definitions(-DGRINPP_USER_AGENT="Grin++ ${PROJECT_VERSION}")
#endif()

include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include/ThirdParty/mio/include
	${PROJECT_SOURCE_DIR}/include/ThirdParty/stduuid/include
	${PROJECT_SOURCE_DIR}/include/ThirdParty/lru
	${PROJECT_SOURCE_DIR}/include/ThirdParty/asio/asio/include
	${PROJECT_SOURCE_DIR}/include/ThirdParty/ghc
	${PROJECT_SOURCE_DIR}/include/ThirdParty/rocksdb/include
	${PROJECT_SOURCE_DIR}/include/ThirdParty/jsoncpp
	${PROJECT_SOURCE_DIR}/include/ThirdParty/zlib
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# Dependencies
# RocksDB
set(WITH_TESTS FALSE)
include_directories(include/ThirdParty/rocksdb)
add_subdirectory(include/ThirdParty/rocksdb)

if(APPLE)
	set(ROCKS_DB_LIB rocksdb-shared${ARTIFACT_SUFFIX})
else()
	set(ROCKS_DB_LIB rocksdb${ARTIFACT_SUFFIX})
endif()

# SQLite3
add_subdirectory(include/ThirdParty/libsqlite3)

# ZLib
add_subdirectory(include/ThirdParty/zlib)

# Grin++ Projects
add_subdirectory(Infrastructure)
add_subdirectory(Crypto)
add_subdirectory(Core)
add_subdirectory(Config)
add_subdirectory(PMMR)
add_subdirectory(PoW)
add_subdirectory(Database)
add_subdirectory(BlockChain)
add_subdirectory(TxPool)
add_subdirectory(P2P)
add_subdirectory(Wallet)
add_subdirectory(Server)

add_executable(RunAllTests RunAllTests.cpp)
