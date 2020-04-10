#include <catch.hpp>

#include <PMMR/Common/PruneList.h>

TEST_CASE("PruneList::IsPruned")
{
	std::shared_ptr<PruneList> pPruneList = PruneList::Load("C:\\FakeFile.txt");
	pPruneList->Add(0);
	pPruneList->Add(1);
	pPruneList->Add(4);
	pPruneList->Add(7);
	REQUIRE(pPruneList->IsPruned(0));
	REQUIRE(pPruneList->IsPruned(1));
	REQUIRE(pPruneList->IsPruned(2));
	REQUIRE(!pPruneList->IsPruned(3));
	REQUIRE(pPruneList->IsPruned(4));
	REQUIRE(!pPruneList->IsPruned(5));
	REQUIRE(!pPruneList->IsPruned(6));
	REQUIRE(pPruneList->IsPruned(7));
}

TEST_CASE("PruneList::IsPrunedRoot")
{
	std::shared_ptr<PruneList> pPruneList = PruneList::Load("C:\\FakeFile.txt");
	pPruneList->Add(0);
	pPruneList->Add(1);
	pPruneList->Add(4);
	pPruneList->Add(7);
	REQUIRE(!pPruneList->IsPrunedRoot(0));
	REQUIRE(!pPruneList->IsPrunedRoot(1));
	REQUIRE(pPruneList->IsPrunedRoot(2));
	REQUIRE(!pPruneList->IsPrunedRoot(3));
	REQUIRE(pPruneList->IsPrunedRoot(4));
	REQUIRE(!pPruneList->IsPrunedRoot(5));
	REQUIRE(!pPruneList->IsPrunedRoot(6));
	REQUIRE(pPruneList->IsPrunedRoot(7));
}