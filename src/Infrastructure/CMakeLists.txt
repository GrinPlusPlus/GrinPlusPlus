set(TARGET_NAME Infrastructure)

file(GLOB SOURCE_CODE
    "LoggerImpl.cpp"
    "ThreadManagerImpl.cpp"
	"ShutdownManagerImpl.cpp"
)

include_directories(${PROJECT_SOURCE_DIR}/deps/spdlog)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
else(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_link_libraries(${TARGET_NAME})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_INFRASTRUCTURE)