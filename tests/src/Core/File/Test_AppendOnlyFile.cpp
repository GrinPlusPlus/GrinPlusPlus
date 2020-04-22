#include <catch.hpp>

#include <Core/File/DataFile.h>
#include <TestFileUtil.h>
#include <Crypto/RandomNumberGenerator.h>

static void Test(const TemporaryFile::Ptr& pFile)
{

    {
        std::ofstream file(pFile->GetPath(), std::ios::out | std::ios::binary | std::ios::app);
        REQUIRE(file.is_open());

        file.seekp(0, std::ios::beg);
        const auto rand = RandomNumberGenerator::GenerateRandom32();
        file.write((const char*)&rand[0], rand.size());
        file.close();
    }

    const size_t fileSize = 16;

    HANDLE hFile = CreateFile(pFile->GetPath().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    LARGE_INTEGER li;
    li.QuadPart = fileSize;
    bool success = SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) && SetEndOfFile(hFile);

    DWORD err = GetLastError();
    LOG_INFO_F("hFile: {} - success: {} - error: {}", (uint64_t)hFile, std::to_string(success), err);

    const auto file_mapping_handle = ::CreateFileMapping(
        hFile,
        0,
        PAGE_READONLY,
        0,
        16,
        0);
    err = GetLastError();
    LOG_INFO_F("hFile: {} - success: {} - error: {}", (uint64_t)hFile, std::to_string(success), err);

    auto pBuf = (LPTSTR)MapViewOfFile(file_mapping_handle,   // handle to map object
        FILE_MAP_READ, // read/write permission
        0,
        0,
        16);
    err = GetLastError();
    LOG_INFO_F("hFile: {} - success: {} - error: {}", (uint64_t)hFile, std::to_string(success), err);

    ::UnmapViewOfFile(pBuf);
    err = GetLastError();
    LOG_INFO_F("hFile: {} - success: {} - error: {}", (uint64_t)hFile, std::to_string(success), err);

    CloseHandle(file_mapping_handle);
    err = GetLastError();
    LOG_INFO_F("hFile: {} - success: {} - error: {}", (uint64_t)hFile, std::to_string(success), err);


    li.QuadPart = 8;
    success = SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) && SetEndOfFile(hFile);

    err = GetLastError();
    LOG_INFO_F("hFile: {} - success: {} - error: {}", (uint64_t)hFile, std::to_string(success), err);

    CloseHandle(hFile);
}

TEST_CASE("DataFile")
{
    auto pFile = TestFileUtil::CreateTempFile();

    //FileUtil::TruncateFile(pFile->GetPath(), 10);

    CBigInteger<32> bigInt = RandomNumberGenerator::GenerateRandom32();

    auto pDataFile = DataFile<32>::Load(pFile->GetPath());

    pDataFile->AddData(bigInt);
    pDataFile->AddData(RandomNumberGenerator::GenerateRandom32());
    pDataFile->AddData(RandomNumberGenerator::GenerateRandom32());
    pDataFile->Commit();

    REQUIRE(pDataFile->GetSize() == 3);

    pDataFile->Rewind(2);
    pDataFile->AddData(RandomNumberGenerator::GenerateRandom32());
    pDataFile->AddData(RandomNumberGenerator::GenerateRandom32());

    REQUIRE(pDataFile->GetDataAt(0) == bigInt.GetData());

    pDataFile->Commit();

    REQUIRE(pDataFile->GetSize() == 4);
}