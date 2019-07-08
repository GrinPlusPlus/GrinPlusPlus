set(TARGET_NAME Core)

file(GLOB CORE_SRC
	"roaring.c"
    "File.cpp"
    "Models/*.cpp"
	"Validation/*.cpp"
	"../include/ThirdParty/jsoncpp/jsoncpp.cpp"
)

add_subdirectory(Tests)
add_library(${TARGET_NAME} STATIC ${CORE_SRC})

if (WIN32)
    set(OS_SPECIFIC_LIBS "")
else(WIN32)
    find_library(OS_SPECIFIC_LIBS pthread)
endif(WIN32)

add_dependencies(${TARGET_NAME} Crypto)
target_link_libraries(${TARGET_NAME} ${OS_SPECIFIC_LIBS} Crypto)
