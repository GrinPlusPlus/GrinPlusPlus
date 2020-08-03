set(TARGET_NAME Net)

file(GLOB SOURCE_CODE
    "Socket.cpp"
    "Servers/Server.cpp"
    "Util/HTTPUtil.cpp"
)

add_subdirectory(Tor)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} Common Core sha3 unofficial-sodium::sodium civetweb::civetweb civetweb::civetweb-cpp)
