#include <catch.hpp>

#include <Net/Tor/TorAddressParser.h>

TEST_CASE("TorAddressParser - Valid Addresses")
{
	{
		std::string address = "grinjoin5pzzisnne3naxx4w2knwxsyamqmzfnzywnzdk7ra766u7vid";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(parsed.has_value());

		std::string hex = parsed.value().GetPublicKey().bytes.ToHex();
		REQUIRE(hex == "3450d4b90debf39449ad26da0bdf96d29b6bcb00641992b738b372357e20ffbd");
	}
	{
		std::string address = "uappxosquocltxoj63zugtnfiocshkxlqswuhyhopjfmupcqnknecuqd";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(parsed.has_value());

		std::string hex = parsed.value().GetPublicKey().bytes.ToHex();
		REQUIRE(hex == "a01efbba50a384b9ddc9f6f3434da5438523aaeb84ad43e0ee7a4aca3c506a9a");
	}
	{
		std::string address = "3qewg3yajasr7xvpnnb5omrj5zdn3jsuxgsibepekaevqkp3vtdgnzad";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(parsed.has_value());
	}
	{
		std::string address = "x5gotnkmtnprdbebfusarbjr6fgkpsapkmkiya37i7l7elsf5ybcryyd";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(parsed.has_value());
	}
}

TEST_CASE("TorAddressParser - Invalid Checksum")
{
	{
		std::string address = "aappxosquocltxoj63zugtnfiocshkxlqswuhyhopjfmupcqnknecuqd";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(!parsed.has_value());
	}
	{
		std::string address = "3qewg3yajasr7xvpnnb4omrj5zdn3jsuxgsibepekaevqkp3vtdgnzad";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(!parsed.has_value());
	}
	{
		std::string address = "x5gotnkmtnprdbebfusarbjr6fgkpsapkmkiya37i7l7elsf5ybcryzd";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(!parsed.has_value());
	}
}

TEST_CASE("TorAddressParser - Invalid length")
{
	{
		std::string address = "uappxosquocltxoj63zugtnfiocshkxlqswuhyhopjfmupcqnknecuqdab";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(!parsed.has_value());
	}
	{
		std::string address = "3qewg3yajasr7xvpnnb4omrj5zdn3jsuxgsibepekaevqkp3vtdgnz";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(!parsed.has_value());
	}
	{
		std::string address = "";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(!parsed.has_value());
	}
}

TEST_CASE("TorAddressParser - Invalid Characters")
{
	{
		std::string address = "UAppxosquocltxoj63zugtnfioc!?#xlqswuhyhopjfmupcqnknecuqd";
		std::optional<TorAddress> parsed = TorAddressParser::Parse(address);
		REQUIRE(!parsed.has_value());
	}
}