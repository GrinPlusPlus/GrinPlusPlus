set(TARGET_NAME Config)

file(GLOB SOURCE_CODE
    "ConfigManager.cpp"
    "ConfigReader.cpp"
    "ConfigWriter.cpp"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_link_libraries(${TARGET_NAME} Core Infrastructure)
target_compile_definitions(${TARGET_NAME} PRIVATE MW_CONFIG)
