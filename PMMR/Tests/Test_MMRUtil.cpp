#include <ThirdParty/Catch2/catch.hpp>

#include "../Common/MMRUtil.h"

TEST_CASE("MMRUtil::GetHeight")
{
	REQUIRE(MMRUtil::GetHeight(0) == 0);
	REQUIRE(MMRUtil::GetHeight(1) == 0);
	REQUIRE(MMRUtil::GetHeight(2) == 1);
	REQUIRE(MMRUtil::GetHeight(3) == 0);
	REQUIRE(MMRUtil::GetHeight(4) == 0);
	REQUIRE(MMRUtil::GetHeight(5) == 1);
	REQUIRE(MMRUtil::GetHeight(6) == 2);
	REQUIRE(MMRUtil::GetHeight(7) == 0);
	REQUIRE(MMRUtil::GetHeight(8) == 0);
	REQUIRE(MMRUtil::GetHeight(9) == 1);
	REQUIRE(MMRUtil::GetHeight(10) == 0);
	REQUIRE(MMRUtil::GetHeight(11) == 0);
	REQUIRE(MMRUtil::GetHeight(12) == 1);
	REQUIRE(MMRUtil::GetHeight(13) == 2);
	REQUIRE(MMRUtil::GetHeight(14) == 3);
	REQUIRE(MMRUtil::GetHeight(15) == 0);
	REQUIRE(MMRUtil::GetHeight(16) == 0);
	REQUIRE(MMRUtil::GetHeight(17) == 1);
	REQUIRE(MMRUtil::GetHeight(18) == 0);
	REQUIRE(MMRUtil::GetHeight(19) == 0);
	REQUIRE(MMRUtil::GetHeight(20) == 1);
	REQUIRE(MMRUtil::GetHeight(21) == 2);
	REQUIRE(MMRUtil::GetHeight(22) == 0);
	REQUIRE(MMRUtil::GetHeight(23) == 0);
	REQUIRE(MMRUtil::GetHeight(24) == 1);
	REQUIRE(MMRUtil::GetHeight(25) == 0);
	REQUIRE(MMRUtil::GetHeight(26) == 0);
	REQUIRE(MMRUtil::GetHeight(27) == 1);
	REQUIRE(MMRUtil::GetHeight(28) == 2);
	REQUIRE(MMRUtil::GetHeight(29) == 3);
	REQUIRE(MMRUtil::GetHeight(30) == 4);
	REQUIRE(MMRUtil::GetHeight(1048574) == 19);
}

TEST_CASE("MMRUtil::GetPeakIndices")
{
	REQUIRE(MMRUtil::GetPeakIndices(0) == std::vector<uint64_t>({ }));
	REQUIRE(MMRUtil::GetPeakIndices(1) == std::vector<uint64_t>({ 0 }));
	REQUIRE(MMRUtil::GetPeakIndices(2) == std::vector<uint64_t>({ }));
	REQUIRE(MMRUtil::GetPeakIndices(3) == std::vector<uint64_t>({ 2 }));
	REQUIRE(MMRUtil::GetPeakIndices(4) == std::vector<uint64_t>({ 2, 3 }));
	REQUIRE(MMRUtil::GetPeakIndices(5) == std::vector<uint64_t>({ }));
	REQUIRE(MMRUtil::GetPeakIndices(6) == std::vector<uint64_t>({ }));
	REQUIRE(MMRUtil::GetPeakIndices(7) == std::vector<uint64_t>({ 6 }));
	REQUIRE(MMRUtil::GetPeakIndices(8) == std::vector<uint64_t>({ 6, 7 }));
	REQUIRE(MMRUtil::GetPeakIndices(9) == std::vector<uint64_t>({ }));
	REQUIRE(MMRUtil::GetPeakIndices(10) == std::vector<uint64_t>({ 6, 9 }));
	REQUIRE(MMRUtil::GetPeakIndices(11) == std::vector<uint64_t>({ 6, 9, 10 }));
	REQUIRE(MMRUtil::GetPeakIndices(22) == std::vector<uint64_t>({ 14, 21 }));
	REQUIRE(MMRUtil::GetPeakIndices(32) == std::vector<uint64_t>({ 30, 31 }));
	REQUIRE(MMRUtil::GetPeakIndices(35) == std::vector<uint64_t>({ 30, 33, 34 }));
	REQUIRE(MMRUtil::GetPeakIndices(42) == std::vector<uint64_t>({ 30, 37, 40, 41 }));
}

TEST_CASE("MMRUtil::GetNumNodes")
{
	REQUIRE(MMRUtil::GetNumNodes(0) == 1);
	REQUIRE(MMRUtil::GetNumNodes(1) == 3);
	REQUIRE(MMRUtil::GetNumNodes(2) == 3);
	REQUIRE(MMRUtil::GetNumNodes(3) == 4);
	REQUIRE(MMRUtil::GetNumNodes(4) == 7);
	REQUIRE(MMRUtil::GetNumNodes(5) == 7);
	REQUIRE(MMRUtil::GetNumNodes(6) == 7);
	REQUIRE(MMRUtil::GetNumNodes(7) == 8);
	REQUIRE(MMRUtil::GetNumNodes(8) == 10);
	REQUIRE(MMRUtil::GetNumNodes(9) == 10);
	REQUIRE(MMRUtil::GetNumNodes(10) == 11);
	REQUIRE(MMRUtil::GetNumNodes(11) == 15);
	REQUIRE(MMRUtil::GetNumNodes(12) == 15);
	REQUIRE(MMRUtil::GetNumNodes(13) == 15);
	REQUIRE(MMRUtil::GetNumNodes(14) == 15);
	REQUIRE(MMRUtil::GetNumNodes(15) == 16);
	REQUIRE(MMRUtil::GetNumNodes(16) == 18);
	REQUIRE(MMRUtil::GetNumNodes(17) == 18);
	REQUIRE(MMRUtil::GetNumNodes(18) == 19);
	REQUIRE(MMRUtil::GetNumNodes(19) == 22);
	REQUIRE(MMRUtil::GetNumNodes(20) == 22);
}

TEST_CASE("MMRUtil::GetParentIndex")
{
	REQUIRE(MMRUtil::GetParentIndex(0) == 2);
	REQUIRE(MMRUtil::GetParentIndex(1) == 2);
	REQUIRE(MMRUtil::GetParentIndex(2) == 6);
	REQUIRE(MMRUtil::GetParentIndex(3) == 5);
	REQUIRE(MMRUtil::GetParentIndex(4) == 5);
	REQUIRE(MMRUtil::GetParentIndex(5) == 6);
	REQUIRE(MMRUtil::GetParentIndex(6) == 14);
	REQUIRE(MMRUtil::GetParentIndex(7) == 9);
	REQUIRE(MMRUtil::GetParentIndex(8) == 9);
	REQUIRE(MMRUtil::GetParentIndex(9) == 13);
	REQUIRE(MMRUtil::GetParentIndex(10) == 12);
	REQUIRE(MMRUtil::GetParentIndex(11) == 12);
	REQUIRE(MMRUtil::GetParentIndex(12) == 13);
	REQUIRE(MMRUtil::GetParentIndex(13) == 14);
	REQUIRE(MMRUtil::GetParentIndex(14) == 30);
	REQUIRE(MMRUtil::GetParentIndex(15) == 17);
}

TEST_CASE("MMRUtil::GetSiblingIndex")
{
	REQUIRE(MMRUtil::GetSiblingIndex(0) == 1);
	REQUIRE(MMRUtil::GetSiblingIndex(1) == 0);
	REQUIRE(MMRUtil::GetSiblingIndex(2) == 5);
	REQUIRE(MMRUtil::GetSiblingIndex(3) == 4);
	REQUIRE(MMRUtil::GetSiblingIndex(4) == 3);
	REQUIRE(MMRUtil::GetSiblingIndex(5) == 2);
	REQUIRE(MMRUtil::GetSiblingIndex(6) == 13);
	REQUIRE(MMRUtil::GetSiblingIndex(7) == 8);
	REQUIRE(MMRUtil::GetSiblingIndex(8) == 7);
	REQUIRE(MMRUtil::GetSiblingIndex(9) == 12);
	REQUIRE(MMRUtil::GetSiblingIndex(10) == 11);
	REQUIRE(MMRUtil::GetSiblingIndex(11) == 10);
	REQUIRE(MMRUtil::GetSiblingIndex(12) == 9);
	REQUIRE(MMRUtil::GetSiblingIndex(13) == 6);
	REQUIRE(MMRUtil::GetSiblingIndex(14) == 29);
	REQUIRE(MMRUtil::GetSiblingIndex(15) == 16);
	REQUIRE(MMRUtil::GetSiblingIndex(16) == 15);
	REQUIRE(MMRUtil::GetSiblingIndex(29) == 14);
}

TEST_CASE("MMRUtil::GetNumLeaves")
{
	REQUIRE(MMRUtil::GetNumLeaves(0) == 1);
	REQUIRE(MMRUtil::GetNumLeaves(1) == 2);
	REQUIRE(MMRUtil::GetNumLeaves(2) == 2);
	REQUIRE(MMRUtil::GetNumLeaves(3) == 3);
	REQUIRE(MMRUtil::GetNumLeaves(4) == 4);
	REQUIRE(MMRUtil::GetNumLeaves(5) == 4);
	REQUIRE(MMRUtil::GetNumLeaves(6) == 4);
	REQUIRE(MMRUtil::GetNumLeaves(7) == 5);
	REQUIRE(MMRUtil::GetNumLeaves(8) == 6);
	REQUIRE(MMRUtil::GetNumLeaves(9) == 6);
	REQUIRE(MMRUtil::GetNumLeaves(10) == 7);
}