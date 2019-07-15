set(TARGET_NAME Database)

file(GLOB DATABASE_SRC
	"*.h"
	"*.cpp"
)

if (WIN32)
    set(OS_LIBS "")
else(WIN32)
	hunter_add_package(gflags)
	find_package(gflags CONFIG REQUIRED)

	set(OS_LIBS gflags)
endif(WIN32)

add_library(${TARGET_NAME} SHARED ${DATABASE_SRC})

target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto)
target_link_libraries(${TARGET_NAME} PUBLIC Infrastructure Core Crypto rocksdb-shared${ARTIFACT_SUFFIX} ${OS_LIBS})