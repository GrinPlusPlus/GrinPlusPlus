set(TARGET_NAME Database)

file(GLOB SOURCE_CODE
	"*.h"
	"*.cpp"
)

if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_DATABASE)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto)
target_link_libraries(${TARGET_NAME} PUBLIC Infrastructure Core Crypto ${ROCKS_DB_LIB})