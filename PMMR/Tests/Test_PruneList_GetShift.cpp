#include <ThirdParty/Catch2/catch.hpp>

#include "../Common/PruneList.h"

// start with an empty prune list (nothing shifted)
TEST_CASE("PruneList::GetShift_Empty")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	// PruneList empty
	REQUIRE(pruneList.GetShift(0) == 0);
	REQUIRE(pruneList.GetShift(1) == 0);
	REQUIRE(pruneList.GetShift(2) == 0);
	REQUIRE(pruneList.GetShift(3) == 0);
	REQUIRE(pruneList.GetTotalShift() == 0);
}

// Add a single leaf to the prune list.
// Nothing is shifted, because shifting only occurs after a parent is pruned.
TEST_CASE("PruneList::GetShift Pruned 0")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(0);
	pruneList.Flush();

	// PruneList contains: 0
	REQUIRE(pruneList.GetShift(0) == 0);
	REQUIRE(pruneList.GetShift(1) == 0);
	REQUIRE(pruneList.GetShift(2) == 0);
	REQUIRE(pruneList.GetShift(3) == 0);
	REQUIRE(pruneList.GetTotalShift() == 0);
}

// Add the first 2 leaves to the prune list.
// The parent node will be pruned, resulting in all leaves after the leaf 0 & 1 to shift by 2.
TEST_CASE("PruneList::GetShift Pruned 0,1")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(0);
	pruneList.Add(1);
	pruneList.Flush();

	// PruneList contains: 2
	REQUIRE(pruneList.GetShift(0) == 0);
	REQUIRE(pruneList.GetShift(1) == 0);
	REQUIRE(pruneList.GetShift(2) == 2);
	REQUIRE(pruneList.GetShift(3) == 2);
	REQUIRE(pruneList.GetShift(4) == 2);
	REQUIRE(pruneList.GetTotalShift() == 2);
}

// Add the first 2 leaves to the prune list. Also, add the parent (node 2) to prune list.
// It should've already been in prune list, so no change should occur.
TEST_CASE("PruneList::GetShift Pruned 0,1,2")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(0);
	pruneList.Add(1);
	pruneList.Add(2);
	pruneList.Flush();

	// PruneList contains: 2
	REQUIRE(pruneList.GetShift(0) == 0);
	REQUIRE(pruneList.GetShift(1) == 0);
	REQUIRE(pruneList.GetShift(2) == 2);
	REQUIRE(pruneList.GetShift(3) == 2);
	REQUIRE(pruneList.GetShift(4) == 2);
	REQUIRE(pruneList.GetTotalShift() == 2);
}

// TODO: Finish this.