#include <catch.hpp>

#include <Consensus.h>

using namespace Consensus;

TEST_CASE("Consensus::GetHeaderVersion")
{
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, 0) == 1);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, 10) == 1);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT / 2 - 1) == 1);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT / 2) == 2);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT / 2 + 1) == 2);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT - 1) == 2);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT) == 3);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, ((YEAR_HEIGHT * 3) / 2) - 1) == 3);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, (YEAR_HEIGHT * 3) / 2) == 4);
	REQUIRE(GetHeaderVersion(EEnvironmentType::MAINNET, (YEAR_HEIGHT * 2) - 1) == 4);

	// v5 not active yet
	REQUIRE_THROWS(GetHeaderVersion(EEnvironmentType::MAINNET, (YEAR_HEIGHT * 2)));
}