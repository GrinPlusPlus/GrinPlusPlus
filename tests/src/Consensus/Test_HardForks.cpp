#include <catch.hpp>

#include <Consensus/HardForks.h>

using namespace Consensus;

TEST_CASE("Consensus::IsValidHeaderVersion")
{
	REQUIRE(IsValidHeaderVersion(EEnvironmentType::MAINNET, 0, 1));
	REQUIRE(IsValidHeaderVersion(EEnvironmentType::MAINNET, 10, 1));
	REQUIRE_FALSE(IsValidHeaderVersion(EEnvironmentType::MAINNET, 10, 2));
	REQUIRE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT / 2 - 1, 1));
	REQUIRE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT / 2, 2));
	REQUIRE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT / 2 + 1, 2));
	REQUIRE_FALSE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT / 2, 1));
	REQUIRE_FALSE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT, 1));
	REQUIRE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT, 3));
	REQUIRE_FALSE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT, 2));
	REQUIRE_FALSE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT, 1));
	REQUIRE_THROWS(IsValidHeaderVersion(EEnvironmentType::MAINNET, (YEAR_HEIGHT * 3) / 2, 2));
	REQUIRE_FALSE(IsValidHeaderVersion(EEnvironmentType::MAINNET, YEAR_HEIGHT + 1, 2));
}

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

	// v4 not active yet
	REQUIRE_THROWS(GetHeaderVersion(EEnvironmentType::MAINNET, (YEAR_HEIGHT * 3) / 2));
}