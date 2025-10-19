set(TARGET_NAME PoW)

add_library(${TARGET_NAME} STATIC
	"Cuckaroo.cpp"
	"Cuckarood.cpp"
	"Cuckaroom.cpp"
	"Cuckarooz.cpp"
	"Cuckatoo.cpp"
	"DifficultyCalculator.cpp"
	"DifficultyLoader.cpp"
	"PoWValidator.cpp"
	"uint128.cpp"
)
target_compile_definitions(${TARGET_NAME} PRIVATE MW_POW)
target_link_libraries(${TARGET_NAME} Common Core Crypto)