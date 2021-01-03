#include <catch.hpp>

#include <Consensus.h>

using namespace Consensus;

TEST_CASE("Consensus::GetMaxBlockTime")
{
	auto now = std::chrono::system_clock::now();

	REQUIRE(GetMaxBlockTime(now) == (now + std::chrono::seconds(720)).time_since_epoch().count());
}

TEST_CASE("Consensus::GetMaxCoinbaseHeight")
{
	REQUIRE(GetMaxCoinbaseHeight(Environment::MAINNET, 1440) == 0);
	REQUIRE(GetMaxCoinbaseHeight(Environment::MAINNET, 1441) == 1);
}

TEST_CASE("Consensus::GetHorizonHeight")
{
	REQUIRE(GetHorizonHeight(10080) == 0);
	REQUIRE(GetHorizonHeight(10081) == 1);
}