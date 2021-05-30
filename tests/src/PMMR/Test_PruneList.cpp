#include <catch.hpp>

#include <PMMR/Common/PruneList.h>

TEST_CASE("PruneList::IsPruned")
{
	std::shared_ptr<PruneList> pPruneList = PruneList::Load("C:\\FakeFile.txt");
	pPruneList->Add(Index::At(0));
	pPruneList->Add(Index::At(1));
	pPruneList->Add(Index::At(4));
	pPruneList->Add(Index::At(7));
	REQUIRE(pPruneList->IsPruned(Index::At(0)));
	REQUIRE(pPruneList->IsPruned(Index::At(1)));
	REQUIRE(pPruneList->IsPruned(Index::At(2)));
	REQUIRE(!pPruneList->IsPruned(Index::At(3)));
	REQUIRE(pPruneList->IsPruned(Index::At(4)));
	REQUIRE(!pPruneList->IsPruned(Index::At(5)));
	REQUIRE(!pPruneList->IsPruned(Index::At(6)));
	REQUIRE(pPruneList->IsPruned(Index::At(7)));
}

TEST_CASE("PruneList::IsPrunedRoot")
{
	std::shared_ptr<PruneList> pPruneList = PruneList::Load("C:\\FakeFile.txt");
	pPruneList->Add(Index::At(0));
	pPruneList->Add(Index::At(1));
	pPruneList->Add(Index::At(4));
	pPruneList->Add(Index::At(7));
	REQUIRE(!pPruneList->IsPrunedRoot(Index::At(0)));
	REQUIRE(!pPruneList->IsPrunedRoot(Index::At(1)));
	REQUIRE(pPruneList->IsPrunedRoot(Index::At(2)));
	REQUIRE(!pPruneList->IsPrunedRoot(Index::At(3)));
	REQUIRE(pPruneList->IsPrunedRoot(Index::At(4)));
	REQUIRE(!pPruneList->IsPrunedRoot(Index::At(5)));
	REQUIRE(!pPruneList->IsPrunedRoot(Index::At(6)));
	REQUIRE(pPruneList->IsPrunedRoot(Index::At(7)));
}