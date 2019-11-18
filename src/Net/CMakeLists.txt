set(TARGET_NAME Net)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W2 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2 /WX")
    endif()
endif()

file(GLOB SOURCE_CODE
    "Socket.cpp"
)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
add_dependencies(${TARGET_NAME} Infrastructure Core)
target_link_libraries(${TARGET_NAME} Infrastructure Core)
