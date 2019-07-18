set(TARGET_NAME Database)

file(GLOB DATABASE_SRC
	"*.h"
	"*.cpp"
)

add_library(${TARGET_NAME} SHARED ${DATABASE_SRC})

target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto)
target_link_libraries(${TARGET_NAME} PUBLIC Infrastructure Core Crypto ${ROCKS_DB_LIB})