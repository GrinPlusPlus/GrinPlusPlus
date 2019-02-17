#include <ThirdParty/Catch2/catch.hpp>

#include "../Common/PruneList.h"

TEST_CASE("PruneList::IsPruned")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");
	pruneList.Add(0);
	pruneList.Add(1);
	pruneList.Add(4);
	pruneList.Add(7);
	REQUIRE(pruneList.IsPruned(0));
	REQUIRE(pruneList.IsPruned(1));
	REQUIRE(pruneList.IsPruned(2));
	REQUIRE(!pruneList.IsPruned(3));
	REQUIRE(pruneList.IsPruned(4));
	REQUIRE(!pruneList.IsPruned(5));
	REQUIRE(!pruneList.IsPruned(6));
	REQUIRE(pruneList.IsPruned(7));
}

TEST_CASE("PruneList::IsPrunedRoot")
{
	PruneList pruneList = PruneList::Load("C:\\FakeFile.txt");
	pruneList.Add(0);
	pruneList.Add(1);
	pruneList.Add(4);
	pruneList.Add(7);
	REQUIRE(!pruneList.IsPrunedRoot(0));
	REQUIRE(!pruneList.IsPrunedRoot(1));
	REQUIRE(pruneList.IsPrunedRoot(2));
	REQUIRE(!pruneList.IsPrunedRoot(3));
	REQUIRE(pruneList.IsPrunedRoot(4));
	REQUIRE(!pruneList.IsPrunedRoot(5));
	REQUIRE(!pruneList.IsPrunedRoot(6));
	REQUIRE(pruneList.IsPrunedRoot(7));
}

//TEST_CASE("PruneList::PMMR_PRUN")
//{
//	Config config;
//	const std::string outputPruneListPath = config.GetTxHashSetDirectory() + "output/pmmr_prun.bin";
//	PruneList pruneList = PruneList::Load(outputPruneListPath);
//	const uint64_t shift = pruneList.GetShift(5000);
//	const uint64_t leafShift = pruneList.GetLeafShift(5000);
//	REQUIRE(shift == 0);
//}