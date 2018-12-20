cmake_minimum_required(VERSION 3.1)

# HunterGate Package Manager
set(HUNTER_ROOT ${CMAKE_BINARY_DIR})
set(HUNTER_STATUS_DEBUG TRUE)
include("HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.49.tar.gz"
    SHA1 "777200954f3851322932f67c782fe92990b32ab2"
)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_REVISION 1)

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
endif()

add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Grin++ Projects
add_subdirectory(Infrastructure)
add_subdirectory(Crypto)
add_subdirectory(Core)
add_subdirectory(Config)
add_subdirectory(PMMR)
add_subdirectory(Database)
add_subdirectory(BlockChain)
add_subdirectory(P2P)
add_subdirectory(Server)