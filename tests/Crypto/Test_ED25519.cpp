#include <catch.hpp>

#include <Crypto/Crypto.h>
#include <Crypto/ED25519.h>
#include <Crypto/RandomNumberGenerator.h>

TEST_CASE("ED25519 Signature")
{
	const SecretKey secretKey = RandomNumberGenerator::GenerateRandom32();
	const ed25519_public_key_t publicKey = ED25519::CalculatePubKey(secretKey);
	const std::string messageStr = "MESSAGE";
	std::vector<unsigned char> message(messageStr.begin(), messageStr.end());

	Signature signature = ED25519::Sign(secretKey, publicKey, message);

	const bool valid = ED25519::VerifySignature(publicKey, signature, message);
	REQUIRE(valid == true);

	const std::string wrongMessageStr = "WRONG_MESSAGE";
	std::vector<unsigned char> wrongMessage(wrongMessageStr.begin(), wrongMessageStr.end());
	const bool wrongMessageResult = ED25519::VerifySignature(publicKey, signature, wrongMessage);
	REQUIRE(wrongMessageResult == false);

	const ed25519_public_key_t publicKey2 = ED25519::CalculatePubKey(RandomNumberGenerator::GenerateRandom32());
	const bool differentPublicKey = ED25519::VerifySignature(publicKey2, signature, message);
	REQUIRE(differentPublicKey == false);
}