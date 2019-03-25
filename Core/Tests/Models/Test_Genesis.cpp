#include <ThirdParty/Catch2/catch.hpp>

#include <Config/Genesis.h>
#include <Crypto/Crypto.h>

TEST_CASE("Mainnet Genesis")
{
	const FullBlock& genesis = Genesis::MAINNET_GENESIS;
	REQUIRE(genesis.GetHash() == Hash::FromHex("40adad0aec27797b48840aa9e00472015c21baea118ce7a2ff1a82c0f8f5bf82"));

	Serializer serializer;
	genesis.Serialize(serializer);
	const Hash fullHash = Crypto::Blake2b(serializer.GetBytes());
	REQUIRE(fullHash == Hash::FromHex("6be6f34b657b785e558e85cc3b8bdb5bcbe8c10e7e58524c8027da7727e189ef"));
}

TEST_CASE("Floonet Genesis")
{
	const FullBlock& genesis = Genesis::FLOONET_GENESIS;
	REQUIRE(genesis.GetHash() == Hash::FromHex("edc758c1370d43e1d733f70f58cf187c3be8242830429b1676b89fd91ccf2dab"));

	Serializer serializer;
	genesis.Serialize(serializer);
	const Hash fullHash = Crypto::Blake2b(serializer.GetBytes());
	REQUIRE(fullHash == Hash::FromHex("91c638fc019a54e6652bd6bb3d9c5e0c17e889cef34a5c28528e7eb61a884dc4"));
}