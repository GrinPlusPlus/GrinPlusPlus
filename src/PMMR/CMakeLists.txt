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
	"UBMT.cpp"
    "Common/Index.cpp"
    "Common/LeafSet.cpp"
    "Common/MMRHashUtil.cpp"
    "Common/MMRUtil.cpp"
    "Common/PruneList.cpp"
    "Zip/TxHashSetZip.cpp"
    "Zip/ZipFile.cpp"
    "Zip/Zipper.cpp"
)


add_library(${TARGET_NAME} STATIC ${SOURCE_CODE})
target_link_libraries(${TARGET_NAME} Common Crypto Core ZLIB::ZLIB minizip::minizip jsoncpp)