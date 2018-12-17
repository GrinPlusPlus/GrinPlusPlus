set(TARGET_NAME Config)

file(GLOB CONFIG_SRC
    "ConfigManager.cpp"
    "ConfigReader.cpp"
    "ConfigWriter.cpp"
)

hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)

add_library(${TARGET_NAME} SHARED ${CONFIG_SRC})

target_link_libraries(${TARGET_NAME} Core Infrastructure jsoncpp_lib_static)
target_compile_definitions(${TARGET_NAME} PRIVATE MW_CONFIG)
