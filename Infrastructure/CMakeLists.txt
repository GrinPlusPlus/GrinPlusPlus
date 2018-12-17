set(TARGET_NAME Infrastructure)

file(GLOB INFRASTRUCTURE_SRC
    "LoggerImpl.cpp"
    "ThreadManagerImpl.cpp"
	"ZipFileImpl.cpp"
)

add_library(${TARGET_NAME} SHARED ${INFRASTRUCTURE_SRC})

target_link_libraries(${TARGET_NAME})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_INFRASTRUCTURE)