#include <catch.hpp>

#include <TestServer.h>

#include <BlockChain/Chain.h>

TEST_CASE("Chain Batching")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	auto chain_path = pTestServer->GenerateTempDir() / "candidate.chain";
	auto pAllocator = std::make_shared<BlockIndexAllocator>();
	auto pGenesisIndex = pAllocator->GetOrCreateIndex(Global::GetGenesisHash(), 0);

	Hash hash1 = CSPRNG::GenerateRandom32();
	Hash hash2a = CSPRNG::GenerateRandom32();
	Hash hash2b = CSPRNG::GenerateRandom32();
	Hash hash3b = CSPRNG::GenerateRandom32();
	Hash hash4b = CSPRNG::GenerateRandom32();

	Locked<Chain> chain(Chain::Load(pAllocator, EChainType::CANDIDATE, chain_path, pGenesisIndex));

	{
		auto pBatch = chain.BatchWrite();
		pBatch->AddBlock(hash1, 1);
		pBatch->AddBlock(hash2a, 2);
		pBatch->Commit();
	}

	{
		auto pBatch = chain.BatchWrite();
		pBatch->Rewind(1);
		pBatch->AddBlock(hash2b, 2);
		pBatch->Rollback();
	}

	{
		auto pBatch = chain.BatchWrite();
		pBatch->Rewind(1);
		pBatch->AddBlock(hash2b, 2);
		pBatch->AddBlock(hash3b, 3);
		pBatch->AddBlock(hash4b, 4);
		pBatch->Commit();
	}

	{
		REQUIRE_THROWS(chain.BatchWrite()->AddBlock(hash4b, 4));
	}

	{
		auto pReader = chain.Read();
		REQUIRE(pReader->GetHeight() == 4);
		REQUIRE(pReader->GetTipHash() == hash4b);
		for (size_t i = 0; i <= 4; i++)
		{
			REQUIRE(pReader->GetByHeight(i)->GetHeight() == i);
		}

		REQUIRE(pReader->GetByHeight(0)->GetHash() == Global::GetGenesisHash());
		REQUIRE(pReader->GetByHeight(1)->GetHash() == hash1);
		REQUIRE(pReader->GetByHeight(2)->GetHash() == hash2b);
		REQUIRE(pReader->GetByHeight(3)->GetHash() == hash3b);
		REQUIRE(pReader->GetByHeight(4)->GetHash() == hash4b);
	}
}