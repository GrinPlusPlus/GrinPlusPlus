#include <catch.hpp>

#include <Core/File/DataFile.h>
#include <TestFileUtil.h>
#include <Crypto/CSPRNG.h>

TEST_CASE("DataFile")
{
    auto pFile = TestFileUtil::CreateTempFile();

    //FileUtil::TruncateFile(pFile->GetPath(), 10);

    CBigInteger<32> bigInt = CSPRNG::GenerateRandom32();

    auto pDataFile = DataFile<32>::Load(pFile->GetPath());

    pDataFile->AddData(bigInt);
    pDataFile->AddData(CSPRNG::GenerateRandom32());
    pDataFile->AddData(CSPRNG::GenerateRandom32());
    pDataFile->Commit();

    REQUIRE(pDataFile->GetSize() == 3);

    pDataFile->Rewind(2);
    pDataFile->AddData(CSPRNG::GenerateRandom32());
    pDataFile->AddData(CSPRNG::GenerateRandom32());

    REQUIRE(pDataFile->GetDataAt(0) == bigInt.GetData());

    pDataFile->Commit();

    REQUIRE(pDataFile->GetSize() == 4);
}