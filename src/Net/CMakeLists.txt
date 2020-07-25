set(TARGET_NAME Net)

file(GLOB SOURCE_CODE
    "Socket.cpp"
)

add_subdirectory(Tor)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
add_dependencies(${TARGET_NAME} Infrastructure Core sha3)
target_link_libraries(${TARGET_NAME} Infrastructure Core sha3 sodium)
