set(TARGET_NAME Core_Tests)

file(GLOB SOURCE_CODE
    "*.cpp"
	"Models/*.cpp"
	"File/*.cpp"
)

add_executable(${TARGET_NAME} ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} Core TestUtil Wallet BlockChain)