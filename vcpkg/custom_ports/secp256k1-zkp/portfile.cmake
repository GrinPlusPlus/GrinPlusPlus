include(vcpkg_common_functions)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO davidtavarez/secp256k1-zkp
    REF master
    SHA512 175a65dcc8f2801256d6a5689cce1b7e17531fb6bccb34fd6d2c569c73abbcb1126cf4df65fc0f435fdf2804352ccc8f485dde42f5795a9f1e036359a915524c
)

if(WIN32)
		file(COPY ${CURRENT_PORT_DIR}/windows/libsecp256k1-config.h DESTINATION ${SOURCE_PATH})
else()
		#include_directories(${GOBJECT_INCLUDE_DIR})
		file(COPY ${CURRENT_PORT_DIR}/nix/libsecp256k1-config.h DESTINATION ${SOURCE_PATH})
endif()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS_DEBUG
        -DINSTALL_HEADERS=OFF
)

vcpkg_install_cmake()

vcpkg_fixup_cmake_targets()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Handle copyright
file(INSTALL ${SOURCE_PATH}/COPYING DESTINATION ${CURRENT_PACKAGES_DIR}/share/secp256k1-zkp RENAME copyright)
