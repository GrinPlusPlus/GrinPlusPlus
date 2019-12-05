set(TARGET_NAME Core)

file(GLOB SOURCE_CODE
    "File.cpp"
    "Models/*.cpp"
	"Validation/*.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})

if (WIN32)
    set(OS_SPECIFIC_LIBS "")
else(WIN32)
    find_library(OS_SPECIFIC_LIBS pthread)
endif(WIN32)

add_dependencies(${TARGET_NAME} Crypto jsoncpp croaring fmt)
target_link_libraries(${TARGET_NAME} ${OS_SPECIFIC_LIBS} Crypto jsoncpp croaring fmt)
