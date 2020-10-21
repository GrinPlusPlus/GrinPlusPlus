set(TARGET_NAME Tor)

file(GLOB SOURCE_CODE
	"TorAddressParser.cpp"
	"TorProcess.cpp"
    "TorControl.cpp"
    "TorControlClient.cpp"
	"TorConnection.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} reproc++ Common Core Net unofficial-sodium::sodium)
