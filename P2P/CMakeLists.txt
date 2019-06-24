set(TARGET_NAME P2P)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W2 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2 /WX")
    endif()
endif()

file(GLOB P2P_SRC
    "*.cpp"
	"Seed/*.cpp"
	"Sync/*.cpp"
)

add_library(${TARGET_NAME} SHARED ${P2P_SRC})
target_compile_definitions(${TARGET_NAME} PRIVATE MW_P2P)

add_dependencies(${TARGET_NAME} Infrastructure Core Crypto Database)
target_link_libraries(${TARGET_NAME} Infrastructure Core Crypto Database)
