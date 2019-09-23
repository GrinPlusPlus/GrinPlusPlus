set(TARGET_NAME PMMR)

# PMMR
file(GLOB SOURCE_CODE
    "HeaderMMRImpl.cpp"
    "KernelMMR.cpp"
    "OutputPMMR.cpp"
    "RangeProofPMMR.cpp"
    "TxHashSetImpl.cpp"
    "TxHashSetManager.cpp"
    "TxHashSetValidator.cpp"
    "Common/LeafSet.cpp"
    "Common/MMRHashUtil.cpp"
    "Common/MMRUtil.cpp"
    "Common/PruneList.cpp"
    "Zip/TxHashSetZip.cpp"
    "Zip/ZipFile.cpp"
    "Zip/Zipper.cpp"
)


if(GRINPP_STATIC)
	add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
elseif(GRINPP_STATIC)
	add_library(${TARGET_NAME} SHARED ${SOURCE_CODE})
endif(GRINPP_STATIC)

target_compile_definitions(${TARGET_NAME} PRIVATE MW_PMMR)
add_dependencies(${TARGET_NAME} Infrastructure Crypto Core Minizip jsoncpp)
target_link_libraries(${TARGET_NAME} Infrastructure Crypto Core zlibstatic Minizip jsoncpp)