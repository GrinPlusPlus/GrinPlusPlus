set(TARGET_NAME Core)

file(GLOB CORE_SRC
	"roaring.c"
    "File.cpp"
    "Models/*.cpp"
	"Validation/*.cpp"
)

add_subdirectory(Tests)
add_library(${TARGET_NAME} STATIC ${CORE_SRC})

#JsonCPP
hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)
set_target_properties(jsoncpp_lib_static PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

if (WIN32)
    set(OS_SPECIFIC_LIBS "")
else(WIN32)
    find_library(OS_SPECIFIC_LIBS pthread)
endif(WIN32)

add_dependencies(${TARGET_NAME} Crypto)
target_link_libraries(${TARGET_NAME} ${OS_SPECIFIC_LIBS} Crypto jsoncpp_lib_static)
