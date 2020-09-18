set(TARGET_NAME slate_tool)

add_executable(${TARGET_NAME} "slate_tool.cpp")
target_link_libraries(${TARGET_NAME} PRIVATE Core Crypto Wallet)