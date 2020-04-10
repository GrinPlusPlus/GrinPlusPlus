set(TARGET_NAME Consensus_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})