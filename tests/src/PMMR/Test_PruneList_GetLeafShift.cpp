#include <catch.hpp>

#include <TestFileUtil.h>
#include <PMMR/Common/PruneList.h>

// start with an empty prune list (nothing shifted)
TEST_CASE("PruneList::GetLeafShift_Empty")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	// PruneList empty
	REQUIRE(pPruneList->GetLeafShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(2)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(3)) == 0);
}

// Add a single leaf to the prune list.
// Nothing is shifted, because shifting only occurs after a parent is pruned.
TEST_CASE("PruneList::GetLeafShift Pruned 0")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(0));
	pPruneList->Flush();

	// PruneList contains: 0
	REQUIRE(pPruneList->GetLeafShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(2)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(3)) == 0);
}

// Add the first 2 leaves to the prune list.
// The parent node will be pruned, resulting in all leaves after the leaf 0 & 1 to shift by 2.
TEST_CASE("PruneList::GetLeafShift Pruned 0,1")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(0));
	pPruneList->Add(Index::At(1));
	pPruneList->Flush();

	// PruneList contains: 2
	REQUIRE(pPruneList->GetLeafShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(2)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(3)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(4)) == 2);
}

// Add the first 3 leaves (0, 1, 3) to the prune list.
// The parent of leaves 0 & 1 will be pruned, but the parent of node 3 will remain.
TEST_CASE("PruneList::GetLeafShift Pruned 0,1,3")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(0));
	pPruneList->Add(Index::At(1));
	pPruneList->Add(Index::At(3));
	pPruneList->Flush();

	// PruneList contains: 2, 3
	REQUIRE(pPruneList->GetLeafShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(2)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(3)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(4)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(5)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(6)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(7)) == 2);
}

// Add the first 4 leaves (0, 1, 3, 4) to the prune list.
// The parent of leaves 0 & 1 will be pruned along with the parent of leaves 3 & 4.
// Everything after node 5 (parent of leaves 3 & 4) will shift by 4.
TEST_CASE("PruneList::GetLeafShift Pruned 0,1,3,4")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(0));
	pPruneList->Add(Index::At(1));
	pPruneList->Add(Index::At(3));
	pPruneList->Add(Index::At(4));
	pPruneList->Flush();

	// PruneList contains: 6
	REQUIRE(pPruneList->GetLeafShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(2)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(3)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(4)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(5)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(6)) == 4);
	REQUIRE(pPruneList->GetLeafShift(Index::At(7)) == 4);
	REQUIRE(pPruneList->GetLeafShift(Index::At(8)) == 4);
}

// Prune a few unconnected nodes in arbitrary order.
TEST_CASE("PruneList::GetLeafShift Arbitrary pruning")
{
	auto pFile = TestFileUtil::CreateTempFile();
	std::shared_ptr<PruneList> pPruneList = PruneList::Load(pFile->GetPath());

	pPruneList->Add(Index::At(4));
	pPruneList->Add(Index::At(10));
	pPruneList->Add(Index::At(11));
	pPruneList->Add(Index::At(3));
	pPruneList->Flush();

	// PruneList contains: 5, 12
	REQUIRE(pPruneList->GetLeafShift(Index::At(0)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(1)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(2)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(3)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(4)) == 0);
	REQUIRE(pPruneList->GetLeafShift(Index::At(5)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(6)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(7)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(8)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(9)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(10)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(11)) == 2);
	REQUIRE(pPruneList->GetLeafShift(Index::At(12)) == 4);
	REQUIRE(pPruneList->GetLeafShift(Index::At(13)) == 4);
	REQUIRE(pPruneList->GetLeafShift(Index::At(14)) == 4);
}