set(TARGET_NAME Core)

file(GLOB SOURCE_CODE
    "Config.cpp"
    "Global.cpp"
    "File/AppendOnlyFile.cpp"
    "Models/*.cpp"
    "Serialization/Base58.cpp"
    "Traits/Serializable.cpp"
	"Validation/TransactionValidator.cpp"
	"Validation/TxBodyValidator.cpp"
)

if (WIN32)
    set(OS_SPECIFIC_LIBS "")
    set(OS_SPECIFIC_FILES "File/MappedFile_Win.cpp")
else(WIN32)
    find_library(OS_SPECIFIC_LIBS pthread)
    set(OS_SPECIFIC_FILES "File/MappedFile_Nix.cpp")
endif(WIN32)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE} ${OS_SPECIFIC_FILES})
target_link_libraries(${TARGET_NAME} ${OS_SPECIFIC_LIBS} Crypto jsoncpp roaring::roaring bitcoin mio::mio)
