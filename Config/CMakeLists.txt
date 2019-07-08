set(TARGET_NAME Config)

file(GLOB CONFIG_SRC
    "ConfigManager.cpp"
    "ConfigReader.cpp"
    "ConfigWriter.cpp"
	"../include/ThirdParty/jsoncpp/jsoncpp.cpp"
)

add_library(${TARGET_NAME} SHARED ${CONFIG_SRC})

target_link_libraries(${TARGET_NAME} Core Infrastructure)
target_compile_definitions(${TARGET_NAME} PRIVATE MW_CONFIG)
