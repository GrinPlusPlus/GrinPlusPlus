#include <ThirdParty/Catch2/catch.hpp>

#include "../Keychain/KeyChain.h"
#include <Config/ConfigManager.h>

TEST_CASE("KeyChain::KeyDerivation")
{
	Config config = ConfigManager::LoadConfig(EEnvironmentType::MAINNET);
	std::vector<unsigned char> masterSeed = CBigInteger<64>::FromHex("b873212f885ccffbf4692afcb84bc2e55886de2dfa07d90f5c3c239abc31c0a6ce047e30fd8bf6a281e71389aa82d73df74c7bbfb3b06b4639a5cee775cccd3c").GetData();

	KeyChain keyChain = KeyChain::FromSeed(config, (const SecureVector&)masterSeed);

	KeyChainPath keyChainPath1234 = KeyChainPath::FromString("m/1/2/3/4");
	std::unique_ptr<SecretKey> pKey1234 = keyChain.DerivePrivateKey(keyChainPath1234, 1234);
	const CBigInteger<32> expected1234 = CBigInteger<32>::FromHex("f0953d1c040d179ce0c25d0dc9485a0f14761dbdd2ad90af12a9a77fe050df7e");

	REQUIRE(pKey1234->GetBytes() == expected1234);
}