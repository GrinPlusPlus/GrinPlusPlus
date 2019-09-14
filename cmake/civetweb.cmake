set(TARGET_NAME civetweb)

file(GLOB SOURCE_CODE
    "${PROJECT_SOURCE_DIR}/deps/civetweb/src/CivetServer.cpp"
	"${PROJECT_SOURCE_DIR}/deps/civetweb/src/civetweb.c"
)

include_directories(${PROJECT_SOURCE_DIR}/deps/civetweb/include)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})