include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(TestUtil "instance.cpp")
add_dependencies(TestUtil Infrastructure Crypto Core Tor)
target_link_libraries(TestUtil Infrastructure Crypto Core Tor)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src/API)
add_subdirectory(src/BlockChain)
add_subdirectory(src/Consensus)
add_subdirectory(src/Core)
add_subdirectory(src/Crypto)
add_subdirectory(src/Database)
add_subdirectory(src/Net)
add_subdirectory(src/PMMR)
add_subdirectory(src/Wallet)