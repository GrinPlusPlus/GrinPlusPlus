include(vcpkg_common_functions)

if (TARGET_TRIPLET MATCHES "^x86")
    message(WARNING "The CRoaring authors recommend users of this lib against using a 32-bit build.")
endif ()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO RoaringBitmap/CRoaring
    REF v0.2.66
    SHA512 84696c174cf35830d8110366b4bb2a6d23ceb4978556585ff13be82f342a7579e13dd4626af3501ae5cdbdd79811ae28e266c35700ba47a07bea912dd1ee3514
    HEAD_REF master
  PATCHES
    undefine_xopen_source.patch
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" ROARING_BUILD_STATIC)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS
        -DROARING_BUILD_STATIC=${ROARING_BUILD_STATIC}
        -DENABLE_ROARING_TESTS=OFF
        -DROARING_DISABLE_AVX=ON
        -DROARING_DISABLE_NEON=ON
        -DROARING_DISABLE_NATIVE=ON
)

vcpkg_install_cmake()

vcpkg_copy_pdbs()

vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

# Handle copyright
configure_file(${SOURCE_PATH}/LICENSE ${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright COPYONLY)

# CMake integration test
vcpkg_test_cmake(PACKAGE_NAME ${PORT})
