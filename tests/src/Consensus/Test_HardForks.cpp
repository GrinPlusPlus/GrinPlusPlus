#include <catch.hpp>

#include <Consensus.h>

using namespace Consensus;

TEST_CASE("Consensus::GetHeaderVersion")
{
	REQUIRE(GetHeaderVersion(Environment::MAINNET, 0) == 1);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, 10) == 1);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, YEAR_HEIGHT / 2 - 1) == 1);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, YEAR_HEIGHT / 2) == 2);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, YEAR_HEIGHT / 2 + 1) == 2);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, YEAR_HEIGHT - 1) == 2);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, YEAR_HEIGHT) == 3);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, ((YEAR_HEIGHT * 3) / 2) - 1) == 3);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, (YEAR_HEIGHT * 3) / 2) == 4);
	REQUIRE(GetHeaderVersion(Environment::MAINNET, (YEAR_HEIGHT * 2) - 1) == 4);

	// v5 not active yet
	REQUIRE_THROWS(GetHeaderVersion(Environment::MAINNET, (YEAR_HEIGHT * 2)));
}