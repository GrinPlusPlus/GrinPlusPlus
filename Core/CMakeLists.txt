set(TARGET_NAME Core)

file(GLOB CORE_SRC
    "*.cpp"
    "Models/*.cpp"
)

add_subdirectory(Tests)
add_library(${TARGET_NAME} STATIC ${CORE_SRC})

add_dependencies(${TARGET_NAME} Crypto)
target_link_libraries(${TARGET_NAME} Crypto)