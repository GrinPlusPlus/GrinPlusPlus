#include <catch.hpp>

#include <TestFileUtil.h>
#include <PMMR/Common/PruneList.h>

// start with an empty prune list (nothing shifted)
TEST_CASE("PruneList::GetShift_Empty")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	// PruneList empty
	REQUIRE(pPruneList->GetShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(2)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(3)) == 0);
	REQUIRE(pPruneList->GetTotalShift() == 0);
}

// Add a single leaf to the prune list.
// Nothing is shifted, because shifting only occurs after a parent is pruned.
TEST_CASE("PruneList::GetShift Pruned 0")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(0));
	pPruneList->Flush();

	// PruneList contains: 0
	REQUIRE(pPruneList->GetShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(2)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(3)) == 0);
	REQUIRE(pPruneList->GetTotalShift() == 0);
}

// Add the first 2 leaves to the prune list.
// The parent node will be pruned, resulting in all leaves after the leaf 0 & 1 to shift by 2.
TEST_CASE("PruneList::GetShift Pruned 0,1")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(0));
	pPruneList->Add(Index::At(1));
	pPruneList->Flush();

	// PruneList contains: 2
	REQUIRE(pPruneList->GetShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(2)) == 2);
	REQUIRE(pPruneList->GetShift(Index::At(3)) == 2);
	REQUIRE(pPruneList->GetShift(Index::At(4)) == 2);
	REQUIRE(pPruneList->GetTotalShift() == 2);
}

// Add the first 2 leaves to the prune list. Also, add the parent (node 2) to prune list.
// It should've already been in prune list, so no change should occur.
TEST_CASE("PruneList::GetShift Pruned 0,1,2")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(0));
	pPruneList->Add(Index::At(1));
	pPruneList->Add(Index::At(2));
	pPruneList->Flush();

	// PruneList contains: 2
	REQUIRE(pPruneList->GetShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetShift(Index::At(2)) == 2);
	REQUIRE(pPruneList->GetShift(Index::At(3)) == 2);
	REQUIRE(pPruneList->GetShift(Index::At(4)) == 2);
	REQUIRE(pPruneList->GetTotalShift() == 2);
}

// TODO: Finish this.