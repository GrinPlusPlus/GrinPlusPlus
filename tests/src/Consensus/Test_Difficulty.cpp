#include <catch.hpp>

#include <Consensus.h>

using namespace Consensus;

TEST_CASE("Consensus::SecondaryPOWRatio")
{
	REQUIRE(SecondaryPOWRatio(0) == 90);
	REQUIRE(SecondaryPOWRatio(1) == 90);
	REQUIRE(SecondaryPOWRatio(89) == 90);
	REQUIRE(SecondaryPOWRatio(90) == 90);
	REQUIRE(SecondaryPOWRatio(91) == 90);
	REQUIRE(SecondaryPOWRatio(179) == 90);
	REQUIRE(SecondaryPOWRatio(180) == 90);
	REQUIRE(SecondaryPOWRatio(181) == 90);

	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT - 1) == 90);
	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT) == 90);
	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT + 1) == 90);

	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT * 2 - 1) == 89);
	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT * 2) == 89);
	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT * 2 + 1) == 89);

	REQUIRE(SecondaryPOWRatio(64000 - 1) == 85);
	REQUIRE(SecondaryPOWRatio(64000) == 85);
	REQUIRE(SecondaryPOWRatio(64000 + 1) == 85);

	REQUIRE(SecondaryPOWRatio(YEAR_HEIGHT) == 45);

	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT * 91 - 1) == 12);
	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT * 91) == 12);
	REQUIRE(SecondaryPOWRatio(WEEK_HEIGHT * 91 + 1) == 12);

	REQUIRE(SecondaryPOWRatio(YEAR_HEIGHT * 2 - 1) == 1);
	REQUIRE(SecondaryPOWRatio(YEAR_HEIGHT * 2) == 0);
	REQUIRE(SecondaryPOWRatio(YEAR_HEIGHT * 2 + 1) == 0);
}