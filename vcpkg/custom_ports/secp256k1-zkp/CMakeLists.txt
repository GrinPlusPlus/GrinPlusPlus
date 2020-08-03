cmake_minimum_required(VERSION 3.8)
project(secp256k1-zkp C)

option(INSTALL_HEADERS "Install header files" ON)

add_definitions(
    -DENABLE_MODULE_ECDH
    -DENABLE_MODULE_RECOVERY
    -DHAVE_CONFIG_H
)

file(GLOB SOURCES src/secp256k1.c)
add_library(secp256k1-zkp ${SOURCES})
add_library(secp256k1-zkp::secp256k1-zkp ALIAS secp256k1-zkp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS secp256k1-zkp EXPORT secp256k1-zkp-config
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

if(INSTALL_HEADERS)
    file(GLOB HEADERS include/*.h)
    install(FILES ${HEADERS} DESTINATION include/secp256k1-zkp)
endif()


install(
    EXPORT secp256k1-zkp-config
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/secp256k1-zkp
    NAMESPACE ${PROJECT_NAME}::
)
