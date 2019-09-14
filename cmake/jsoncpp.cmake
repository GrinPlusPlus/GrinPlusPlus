set(TARGET_NAME jsoncpp)

file(GLOB JSONCPP_SRC
    "${PROJECT_SOURCE_DIR}/deps/jsoncpp/jsoncpp.cpp"
)

add_library(${TARGET_NAME} STATIC ${JSONCPP_SRC})