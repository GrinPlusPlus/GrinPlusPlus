#include <ThirdParty/Catch2/catch.hpp>

#include "../Common/PruneList.h"

// start with an empty prune list (nothing shifted)
TEST_CASE("PruneList::GetLeafShift_Empty")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	// PruneList empty
	REQUIRE(pruneList.GetLeafShift(0) == 0);
	REQUIRE(pruneList.GetLeafShift(1) == 0);
	REQUIRE(pruneList.GetLeafShift(2) == 0);
	REQUIRE(pruneList.GetLeafShift(3) == 0);
}

// Add a single leaf to the prune list.
// Nothing is shifted, because shifting only occurs after a parent is pruned.
TEST_CASE("PruneList::GetLeafShift Pruned 0")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(0);
	pruneList.Flush();

	// PruneList contains: 0
	REQUIRE(pruneList.GetLeafShift(0) == 0);
	REQUIRE(pruneList.GetLeafShift(1) == 0);
	REQUIRE(pruneList.GetLeafShift(2) == 0);
	REQUIRE(pruneList.GetLeafShift(3) == 0);
}

// Add the first 2 leaves to the prune list.
// The parent node will be pruned, resulting in all leaves after the leaf 0 & 1 to shift by 2.
TEST_CASE("PruneList::GetLeafShift Pruned 0,1")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(0);
	pruneList.Add(1);
	pruneList.Flush();

	// PruneList contains: 2
	REQUIRE(pruneList.GetLeafShift(0) == 0);
	REQUIRE(pruneList.GetLeafShift(1) == 0);
	REQUIRE(pruneList.GetLeafShift(2) == 2);
	REQUIRE(pruneList.GetLeafShift(3) == 2);
	REQUIRE(pruneList.GetLeafShift(4) == 2);
}

// Add the first 3 leaves (0, 1, 3) to the prune list.
// The parent of leaves 0 & 1 will be pruned, but the parent of node 3 will remain.
TEST_CASE("PruneList::GetLeafShift Pruned 0,1,3")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(0);
	pruneList.Add(1);
	pruneList.Add(3);
	pruneList.Flush();

	// PruneList contains: 2, 3
	REQUIRE(pruneList.GetLeafShift(0) == 0);
	REQUIRE(pruneList.GetLeafShift(1) == 0);
	REQUIRE(pruneList.GetLeafShift(2) == 2);
	REQUIRE(pruneList.GetLeafShift(3) == 2);
	REQUIRE(pruneList.GetLeafShift(4) == 2);
	REQUIRE(pruneList.GetLeafShift(5) == 2);
	REQUIRE(pruneList.GetLeafShift(6) == 2);
	REQUIRE(pruneList.GetLeafShift(7) == 2);
}

// Add the first 4 leaves (0, 1, 3, 4) to the prune list.
// The parent of leaves 0 & 1 will be pruned along with the parent of leaves 3 & 4.
// Everything after node 5 (parent of leaves 3 & 4) will shift by 4.
TEST_CASE("PruneList::GetLeafShift Pruned 0,1,3,4")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(0);
	pruneList.Add(1);
	pruneList.Add(3);
	pruneList.Add(4);
	pruneList.Flush();

	// PruneList contains: 6
	REQUIRE(pruneList.GetLeafShift(0) == 0);
	REQUIRE(pruneList.GetLeafShift(1) == 0);
	REQUIRE(pruneList.GetLeafShift(2) == 0);
	REQUIRE(pruneList.GetLeafShift(3) == 0);
	REQUIRE(pruneList.GetLeafShift(4) == 0);
	REQUIRE(pruneList.GetLeafShift(5) == 0);
	REQUIRE(pruneList.GetLeafShift(6) == 4);
	REQUIRE(pruneList.GetLeafShift(7) == 4);
	REQUIRE(pruneList.GetLeafShift(8) == 4);
}

// Prune a few unconnected nodes in arbitrary order.
TEST_CASE("PruneList::GetLeafShift Arbitrary pruning")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");

	pruneList.Add(4);
	pruneList.Add(10);
	pruneList.Add(11);
	pruneList.Add(3);
	pruneList.Flush();

	// PruneList contains: 5, 12
	REQUIRE(pruneList.GetLeafShift(0) == 0);
	REQUIRE(pruneList.GetLeafShift(1) == 0);
	REQUIRE(pruneList.GetLeafShift(2) == 0);
	REQUIRE(pruneList.GetLeafShift(3) == 0);
	REQUIRE(pruneList.GetLeafShift(4) == 0);
	REQUIRE(pruneList.GetLeafShift(5) == 2);
	REQUIRE(pruneList.GetLeafShift(6) == 2);
	REQUIRE(pruneList.GetLeafShift(7) == 2);
	REQUIRE(pruneList.GetLeafShift(8) == 2);
	REQUIRE(pruneList.GetLeafShift(9) == 2);
	REQUIRE(pruneList.GetLeafShift(10) == 2);
	REQUIRE(pruneList.GetLeafShift(11) == 2);
	REQUIRE(pruneList.GetLeafShift(12) == 4);
	REQUIRE(pruneList.GetLeafShift(13) == 4);
	REQUIRE(pruneList.GetLeafShift(14) == 4);
}