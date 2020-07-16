set(TARGET_NAME chachapoly)

file(GLOB SOURCE_CODE
    "${CMAKE_CURRENT_LIST_DIR}/chacha.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/chachapoly.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/poly1305.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})