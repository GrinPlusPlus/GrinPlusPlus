set(TARGET_NAME Common)

file(GLOB SOURCE_CODE
    "ChildProcess.cpp"
    "GrinStr.cpp"
    "Logger.cpp"
    "Secure.cpp"
    "Util/FileUtil.cpp"
    "Util/HexUtil.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_compile_definitions(${TARGET_NAME} PRIVATE SPDLOG_WCHAR_FILENAMES)
target_link_libraries(${TARGET_NAME} PUBLIC reproc++ fmt::fmt-header-only)