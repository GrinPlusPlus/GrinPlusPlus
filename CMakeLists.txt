cmake_minimum_required(VERSION 3.8)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")

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

set(VERSION_MAJOR 0)
set(VERSION_MINOR 6)
set(VERSION_REVISION 5)

project(GRIN_PLUS_PLUS VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

message("Grin++ Version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Setting Clang flags")

if(MSVC)
    message("Setting flags for Visual Studio C++")
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
elseif(APPLE)
	message("Setting flags for XCode")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -fno-aligned-allocation -g -Wall")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "[cC][lL][aA][nN][gG]") #Case insensitive match
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -stdlib=libc++ -fno-aligned-allocation")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} -static-libstdc++ -static-libgcc")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-copy -Wno-pessimizing-move -Wno-maybe-uninitialized")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(WITH_GFLAGS OFF)
#add_definitions(-DNDEBUG)
add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_definitions(-DNOMINMAX)
#add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
#add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
add_definitions(-DASIO_STANDALONE)
add_definitions(-DGRINPP_USER_AGENT="Grin++ ${PROJECT_VERSION}")

include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/deps
	${PROJECT_SOURCE_DIR}/deps/mio/include
	${PROJECT_SOURCE_DIR}/deps/stduuid/include
	${PROJECT_SOURCE_DIR}/deps/lru
	${PROJECT_SOURCE_DIR}/deps/asio/asio/include
	${PROJECT_SOURCE_DIR}/deps/ghc
	${PROJECT_SOURCE_DIR}/deps/rocksdb/include
	${PROJECT_SOURCE_DIR}/deps/jsoncpp
	${PROJECT_SOURCE_DIR}/deps/zlib
	${PROJECT_SOURCE_DIR}/deps/Catch2
	${PROJECT_SOURCE_DIR}/deps/ed25519-donna
	${PROJECT_SOURCE_DIR}/deps/cppcodec
	${CMAKE_CURRENT_BINARY_DIR}/deps/zlib
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# Dependencies
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(sqlite3)
include(minizip)
include(jsoncpp)
include(zlib)
include(rocksdb)
include(croaring)
include(civetweb)
include(uint128)
include(ctaes)
include(scrypt)
include(secp256k1-zkp)
include(sha3)
add_subdirectory(deps/cuckoo)
add_subdirectory(deps/cppcodec)
add_subdirectory(deps/ed25519-donna)

# Grin++ Projects
set(GRINPP_TESTS true)
set(GRINPP_STATIC true)
if(GRINPP_STATIC)
	add_definitions(-DGRINPP_STATIC)
endif(GRINPP_STATIC)

add_subdirectory(src/Infrastructure)
add_subdirectory(src/Crypto)
add_subdirectory(src/Core)
add_subdirectory(src/Config)
add_subdirectory(src/PMMR)
add_subdirectory(src/PoW)
add_subdirectory(src/Database)
add_subdirectory(src/BlockChain)
add_subdirectory(src/TxPool)
add_subdirectory(src/Net)
add_subdirectory(src/P2P)
add_subdirectory(src/Wallet)
add_subdirectory(src/Server)

if(GRINPP_TESTS)
	add_subdirectory(tests/BlockChain)
	add_subdirectory(tests/Consensus)
	#add_subdirectory(tests/Core)
	#add_subdirectory(tests/Crypto)
	add_subdirectory(tests/Net)
	#add_subdirectory(tests/PMMR)
	#add_subdirectory(tests/Wallet)
	add_subdirectory(tests/Server)
	add_executable(RunAllTests tests/RunAllTests.cpp)
endif(GRINPP_TESTS)

#set(CMAKE_CXX_CPPCHECK "C:/Program Files/Cppcheck/cppcheck.exe")
# Add Analyze with CppCheck target if CppCheck is installed
if(WIN32)
  # Find CppCheck executable
  #find_program(CMAKE_CXX_CPPCHECK cppcheck NAMES cppcheck.exe HINTS $ENV{PROGRAMFILES}/cppcheck)
  #set(CMAKE_CXX_CPPCHECK "C:/Program Files/Cppcheck/cppcheck.exe")

  # If CppCheck executable found
  if(false)#CMAKE_CXX_CPPCHECK)
    # Check CppCheck version
    set(CPP_CHECK_CMD ${CMAKE_CXX_CPPCHECK} --version)
    execute_process(COMMAND ${CPP_CHECK_CMD}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE CPP_CHECK_RESULT
      OUTPUT_VARIABLE CPP_CHECK_VERSION
      ERROR_VARIABLE CPP_CHECK_ERROR)
    
    # Check if version could be extracted
    if(CPP_CHECK_RESULT EQUAL 0)
      # Get number of CPU cores
      include(ProcessorCount)
      ProcessorCount(CPU_CORES)

      # Append desired arguments to CppCheck
      list(
        APPEND CMAKE_CXX_CPPCHECK
          
          # Using the below template will allow jumping to any found error from inside Visual Studio output window by double click
          #"--template '${CMAKE_SOURCE_DIR}/{file}({line}): {severity} ({id}): {message}'" 
          
          # Use all the available CPU cores
          "-j ${CPU_CORES}" 
          
          # Only show found errors
          "--quiet" 
          
          # Desired warning level in CppCheck
          "--enable=style"
          
          # Optional: Specified C++ version
          "--std=c++17"
           
          # Optional: Specified platform
          "--platform=win64"
          
          # Optional: suppression file stored in same directory as the top level CMake script
          #"--suppressions-list=${CMAKE_SOURCE_DIR}/cppcheck_suppressions.txt"
          
          # Optional: Use inline suppressions
          "--inline-suppr"
          
          # Run CppCheck from the working directory, as specified in the add_custom_target command below
          "."
        )
      
      add_custom_target(ANALYZE_CPPCHECK DEPENDS GrinNode
        COMMAND ${CMAKE_CXX_CPPCHECK}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Static code analysis using ${CPP_CHECK_VERSION}"
      )
    endif()
  endif()
endif()