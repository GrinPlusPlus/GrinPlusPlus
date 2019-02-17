#include <ThirdParty/Catch2/catch.hpp>

#include <Core/Models/ShortId.h>

TEST_CASE("ShortId::Create")
{
	{
		const CBigInteger<32> hash = CBigInteger<32>::FromHex("0x81e47a19e6b29b0a65b9591762ce5143ed30d0261e5d24a3201752506b20f15c");
		const CBigInteger<32> blockHash = CBigInteger<32>::ValueOf(0);
		const uint64_t nonce = 0;

		ShortId shortId = ShortId::Create(hash, blockHash, nonce);
		REQUIRE(shortId.GetId() == CBigInteger<6>::FromHex("0x4cc808b62476"));
	}

	{
		const CBigInteger<32> hash = CBigInteger<32>::FromHex("0x3a42e66e46dd7633b57d1f921780a1ac715e6b93c19ee52ab714178eb3a9f673");
		const CBigInteger<32> blockHash = CBigInteger<32>::ValueOf(0);
		const uint64_t nonce = 5;

		ShortId shortId = ShortId::Create(hash, blockHash, nonce);
		REQUIRE(shortId.GetId() == CBigInteger<6>::FromHex("0x02955a094534"));
	}

	{
		const CBigInteger<32> hash = CBigInteger<32>::FromHex("0x3a42e66e46dd7633b57d1f921780a1ac715e6b93c19ee52ab714178eb3a9f673");
		const CBigInteger<32> blockHash = CBigInteger<32>::FromHex("0x81e47a19e6b29b0a65b9591762ce5143ed30d0261e5d24a3201752506b20f15c");
		const uint64_t nonce = 5;

		ShortId shortId = ShortId::Create(hash, blockHash, nonce);
		REQUIRE(shortId.GetId() == CBigInteger<6>::FromHex("0x3e9cde72a687"));
	}
}