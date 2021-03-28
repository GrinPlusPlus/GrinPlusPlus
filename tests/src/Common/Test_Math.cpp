#include <catch.hpp>

#include <Common/Math.h>

TEST_CASE("saturated")
{
	// saturated subtraction
	REQUIRE(saturated_sub<uint64_t>(100, 110) == 0);
	REQUIRE(saturated_sub<uint64_t>(100, 90) == 10);

	// saturated addition
	REQUIRE(saturated_add<uint64_t>(0xffffffffffffff00, 0x00000000000000f0) == 0xfffffffffffffff0);
	REQUIRE(saturated_add<uint64_t>(0xffffffffffffff00, 0x00000000000000ff) == std::numeric_limits<uint64_t>::max());
	REQUIRE(saturated_add<uint64_t>(0xffffffffffffff00, 0x0000000000000100) == std::numeric_limits<uint64_t>::max());
}