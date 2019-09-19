set(TARGET_NAME secp256k1-zkp)

include_directories(
	${PROJECT_SOURCE_DIR}/deps/secp256k1-zkp
	${PROJECT_SOURCE_DIR}/deps/secp256k1-zkp/src
)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
			string(REGEX REPLACE "/W[0-4]" "/W2 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2 /WX")
    endif()
endif()

if(WIN32)
		configure_file(${PROJECT_SOURCE_DIR}/deps/libsecp256k1-config.h ${PROJECT_SOURCE_DIR}/deps/secp256k1-zkp/src/libsecp256k1-config.h COPYONLY)
else()
		include_directories(${GOBJECT_INCLUDE_DIR})
		configure_file(${PROJECT_SOURCE_DIR}/deps/libsecp256k1-config-mac.h ${PROJECT_SOURCE_DIR}/deps/secp256k1-zkp/src/libsecp256k1-config.h COPYONLY)
endif()


add_library(${TARGET_NAME} STATIC ${PROJECT_SOURCE_DIR}/deps/secp256k1-zkp/src/secp256k1.c)
target_compile_definitions(${TARGET_NAME} PRIVATE HAVE_CONFIG_H SECP256K1_BUILD)