set(TARGET_NAME Net)

file(GLOB NET_SRC
	"DNS.cpp"
	"Listener.cpp"
	"Socket.cpp"
	"SocketFactory.cpp"
	"IPAddressUtil.cpp"
)

add_library(${TARGET_NAME} STATIC ${NET_SRC})

target_link_libraries(${TARGET_NAME})