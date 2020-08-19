#include <catch.hpp>

#include <Crypto/ED25519.h>
#include <Crypto/CSPRNG.h>

TEST_CASE("ED25519 Signature")
{
	const SecretKey seed = CSPRNG::GenerateRandom32();
	const ed25519_keypair_t keypair = ED25519::CalculateKeypair(seed);
	const std::string messageStr = "MESSAGE";
	std::vector<unsigned char> message(messageStr.begin(), messageStr.end());

	ed25519_signature_t signature = ED25519::Sign(keypair.secret_key, message);

	const bool valid = ED25519::VerifySignature(keypair.public_key, signature, message);
	REQUIRE(valid == true);

	const std::string wrongMessageStr = "WRONG_MESSAGE";
	std::vector<unsigned char> wrongMessage(wrongMessageStr.begin(), wrongMessageStr.end());
	const bool wrongMessageResult = ED25519::VerifySignature(keypair.public_key, signature, wrongMessage);
	REQUIRE(wrongMessageResult == false);

	const SecretKey seed2 = CSPRNG::GenerateRandom32();
	const ed25519_public_key_t publicKey2 = ED25519::CalculateKeypair(seed2).public_key;
	const bool differentPublicKey = ED25519::VerifySignature(publicKey2, signature, message);
	REQUIRE(differentPublicKey == false);
}