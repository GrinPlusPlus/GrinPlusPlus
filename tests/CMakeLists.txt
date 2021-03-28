find_package(Catch2 CONFIG REQUIRED)

set(test_sources "src/TestMain.cpp")
add_subdirectory(framework)
add_subdirectory(src/API)
add_subdirectory(src/BlockChain)
add_subdirectory(src/Common)
add_subdirectory(src/Consensus)
add_subdirectory(src/Core)
add_subdirectory(src/Crypto)
add_subdirectory(src/Database)
add_subdirectory(src/Net)
add_subdirectory(src/PMMR)
add_subdirectory(src/Wallet)

add_executable(Tests ${test_sources})
target_compile_definitions(Tests PRIVATE INCLUDE_TEST_MATH)

target_link_libraries(Tests PRIVATE
    Catch2::Catch2
    API
    BlockChain
    Common
    Core
    Crypto
    Database
    Net
    Tor
    PMMR
    PoW
    TxPool
    Wallet
)

target_include_directories(Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/include
    ${PROJECT_SOURCE_DIR}/src
)

include(CTest)
include(Catch)
catch_discover_tests(Tests)

#add_custom_command(
#    TARGET Tests POST_BUILD
#    COMMAND ctest -C $<CONFIGURATION> --output-on-failure
#)