set(TARGET_NAME Common)

file(GLOB SOURCE_CODE
    "ChildProcess.cpp"
    "GrinStr.cpp"
    "Secure.cpp"
    "Util/FileUtil.cpp"
    "Util/HexUtil.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})

add_dependencies(${TARGET_NAME} fmt)
target_link_libraries(${TARGET_NAME} fmt)
