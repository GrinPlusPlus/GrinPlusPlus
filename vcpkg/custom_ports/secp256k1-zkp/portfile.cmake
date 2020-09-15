include(vcpkg_common_functions)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mimblewimble/secp256k1-zkp
    REF 5b39fbf7f1e16e126404791923511723c4c0876d
    SHA512 a83b4da2e8e0325cdbda26384ed43d1d4e104c48788fa2a7443786e74dae743c138a2d345d61b991579b61974057e37c6ebff7232140ccd1112f5e3877d8e287
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
