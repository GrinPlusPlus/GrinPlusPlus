set(TARGET_NAME Net)

file(GLOB NET_SRC
	"Socket.cpp"
)

add_library(${TARGET_NAME} STATIC ${NET_SRC})

target_link_libraries(${TARGET_NAME})