set(TARGET_NAME Core)

file(GLOB SOURCE_CODE
    "Config.cpp"
    "Global.cpp"
    "File/AppendOnlyFile.cpp"
    "Models/*.cpp"
    "Serialization/Base58.cpp"
    "Traits/*.cpp"
    "Validation/*.cpp"
)

if (WIN32)
    set(OS_SPECIFIC_LIBS "")
    set(OS_SPECIFIC_FILES "File/MappedFile_Win.cpp")
else(WIN32)
    set (THREADS_PREFER_PTHREAD_FLAG ON)
    find_package (Threads REQUIRED)
    set(OS_SPECIFIC_FILES "File/MappedFile_Nix.cpp")
endif(WIN32)

add_library(${TARGET_NAME} STATIC ${SOURCE_CODE} ${OS_SPECIFIC_FILES})
target_link_libraries(${TARGET_NAME} Threads::Threads Crypto jsoncpp roaring::roaring bitcoin mio::mio)
