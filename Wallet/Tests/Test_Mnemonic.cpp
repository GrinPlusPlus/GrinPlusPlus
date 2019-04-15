#include <ThirdParty/Catch2/catch.hpp>

#include "../Keychain/Mnemonic.h"
#include <Crypto/BigInteger.h>

TEST_CASE("Mnemonic::CreateMnemonic")
{
	{
		std::vector<unsigned char> entropy = CBigInteger<16>::FromHex("00000000000000000000000000000000").GetData();
		SecureString mnemonic = Mnemonic().CreateMnemonic(entropy);
		SecureString expected = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
		REQUIRE(mnemonic == expected);
	}

	{
		std::vector<unsigned char> entropy = CBigInteger<16>::FromHex("7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f").GetData();
		SecureString mnemonic = Mnemonic().CreateMnemonic(entropy);
		SecureString expected = "legal winner thank year wave sausage worth useful legal winner thank yellow";
		REQUIRE(mnemonic == expected);
	}

	{
		std::vector<unsigned char> entropy = CBigInteger<16>::FromHex("80808080808080808080808080808080").GetData();
		SecureString mnemonic = Mnemonic().CreateMnemonic(entropy);
		SecureString expected = "letter advice cage absurd amount doctor acoustic avoid letter advice cage above";
		REQUIRE(mnemonic == expected);
	}

	{
		std::vector<unsigned char> entropy = CBigInteger<16>::FromHex("ffffffffffffffffffffffffffffffff").GetData();
		SecureString mnemonic = Mnemonic().CreateMnemonic(entropy);
		SecureString expected = "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong";
		REQUIRE(mnemonic == expected);
	}

	// TODO: Include remaining test vectors from https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki#Test_vectors

	{
		std::vector<unsigned char> entropy = CBigInteger<32>::FromHex("f585c11aec520db57dd353c69554b21a89b20fb0650966fa0a9d6f74fd989d8f").GetData();
		SecureString mnemonic = Mnemonic().CreateMnemonic(entropy);
		SecureString expected = "void come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold";
		REQUIRE(mnemonic == expected);
	}
}

TEST_CASE("Mnemonic::ToEntropy")
{
	{
		SecureString walletWords = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
		std::optional<SecureVector> entropyOpt = Mnemonic().ToEntropy(walletWords);
		std::vector<unsigned char> expected = CBigInteger<16>::FromHex("00000000000000000000000000000000").GetData();
		REQUIRE(((const std::vector<unsigned char>&)entropyOpt.value()) == expected);
	}

	{
		SecureString walletWords = "legal winner thank year wave sausage worth useful legal winner thank yellow";
		std::optional<SecureVector> entropyOpt = Mnemonic().ToEntropy(walletWords);
		std::vector<unsigned char> expected = CBigInteger<16>::FromHex("7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f").GetData();
		REQUIRE(((const std::vector<unsigned char>&)entropyOpt.value()) == expected);
	}

	{
		SecureString walletWords = "letter advice cage absurd amount doctor acoustic avoid letter advice cage above";
		std::optional<SecureVector> entropyOpt = Mnemonic().ToEntropy(walletWords);
		std::vector<unsigned char> expected = CBigInteger<16>::FromHex("80808080808080808080808080808080").GetData();
		REQUIRE(((const std::vector<unsigned char>&)entropyOpt.value()) == expected);
	}

	{
		SecureString walletWords = "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong";
		std::optional<SecureVector> entropyOpt = Mnemonic().ToEntropy(walletWords);
		std::vector<unsigned char> expected = CBigInteger<16>::FromHex("ffffffffffffffffffffffffffffffff").GetData();
		REQUIRE(((const std::vector<unsigned char>&)entropyOpt.value()) == expected);
	}

	// TODO: Include remaining test vectors from https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki#Test_vectors

	{
		SecureString walletWords = "void come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold";
		std::optional<SecureVector> entropyOpt = Mnemonic().ToEntropy(walletWords);
		std::vector<unsigned char> expected = CBigInteger<32>::FromHex("f585c11aec520db57dd353c69554b21a89b20fb0650966fa0a9d6f74fd989d8f").GetData();
		REQUIRE(((const std::vector<unsigned char>&)entropyOpt.value()) == expected);
	}
}