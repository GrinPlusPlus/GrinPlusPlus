set(TARGET_NAME Tor)

file(GLOB SOURCE_CODE
	"TorManager.cpp"
    "TorControl.cpp"
	"TorConnection.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
add_dependencies(${TARGET_NAME} Infrastructure Core Net)
target_link_libraries(${TARGET_NAME} Infrastructure Core Net)
