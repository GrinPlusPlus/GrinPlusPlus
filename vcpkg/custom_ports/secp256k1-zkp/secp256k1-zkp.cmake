#include ("${CMAKE_CURRENT_LIST_DIR}/asio-targets.cmake")
add_library(secp256k1-zkp::secp256k1-zkp INTERFACE IMPORTED)
target_link_libraries(secp256k1-zkp INTERFACE secp256k1-zkp)

#get_target_property(_ASIO_INCLUDE_DIR asio INTERFACE_INCLUDE_DIRECTORIES)
#set(ASIO_INCLUDE_DIR "${_ASIO_INCLUDE_DIR}")
