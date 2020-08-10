set(TARGET_NAME API)

file(GLOB SOURCE_CODE
    "Node/*.cpp"
    "Wallet/*.cpp"
    "Wallet/Foreign/*.cpp"
    "Wallet/Owner/*.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} Core Net)
