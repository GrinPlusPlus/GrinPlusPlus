#include <catch.hpp>

//#include "../secp256k1-zkp/include/secp256k1_commitment.h"
//#include "../secp256k1-zkp/include/secp256k1_generator.h"

#include "../secp256k1-zkp/include/secp256k1.h"
#include <Crypto/Crypto.h>
#include <Crypto/RandomNumberGenerator.h>

TEST_CASE("AggSig Interaction")
{
	Hash message = RandomNumberGenerator::GenerateRandom32();

	// Generate sender keypairs
	SecretKey secretKeySender = RandomNumberGenerator::GenerateRandom32();
	PublicKey publicKeySender = *Crypto::CalculatePublicKey(secretKeySender);
	SecretKey secretNonceSender = *Crypto::GenerateSecureNonce();
	PublicKey publicNonceSender = *Crypto::CalculatePublicKey(secretNonceSender);

	// Generate receiver keypairs
	SecretKey secretKeyReceiver = RandomNumberGenerator::GenerateRandom32();
	PublicKey publicKeyReceiver = *Crypto::CalculatePublicKey(secretKeyReceiver);
	SecretKey secretNonceReceiver = *Crypto::GenerateSecureNonce();
	PublicKey publicNonceReceiver = *Crypto::CalculatePublicKey(secretNonceReceiver);

	// Add pubKeys and pubNonces
	PublicKey sumPubKeys = *Crypto::AddPublicKeys(std::vector<PublicKey>({ publicKeySender, publicKeyReceiver }));
	PublicKey sumPubNonces = *Crypto::AddPublicKeys(std::vector<PublicKey>({ publicNonceSender, publicNonceReceiver }));

	// Generate partial signatures
	CompactSignature senderPartialSignature = *Crypto::CalculatePartialSignature(secretKeySender, secretNonceSender, sumPubKeys, sumPubNonces, message);
	CompactSignature receiverPartialSignature = *Crypto::CalculatePartialSignature(secretKeyReceiver, secretNonceReceiver, sumPubKeys, sumPubNonces, message);

	// Validate partial signatures
	const bool senderSigValid = Crypto::VerifyPartialSignature(senderPartialSignature, publicKeySender, sumPubKeys, sumPubNonces, message);
	REQUIRE(senderSigValid == true);

	const bool receiverSigValid = Crypto::VerifyPartialSignature(receiverPartialSignature, publicKeyReceiver, sumPubKeys, sumPubNonces, message);
	REQUIRE(receiverSigValid == true);

	// Aggregate signature and validate
	Signature aggregateSignature = *Crypto::AggregateSignatures(std::vector<CompactSignature>({ senderPartialSignature, receiverPartialSignature }), sumPubNonces);
	const bool aggSigValid = Crypto::VerifyAggregateSignature(aggregateSignature, sumPubKeys, message);
	REQUIRE(aggSigValid == true);
}


TEST_CASE("Message Signature")
{
	const SecretKey secretKey = RandomNumberGenerator::GenerateRandom32();
	const std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(secretKey);
	const std::string message = "MESSAGE";

	std::unique_ptr<CompactSignature> pSignature = Crypto::SignMessage(secretKey, *pPublicKey, message);
	REQUIRE(pSignature != nullptr);

	const bool valid = Crypto::VerifyMessageSignature(*pSignature, *pPublicKey, message);
	REQUIRE(valid == true);

	const bool wrongMessage = Crypto::VerifyMessageSignature(*pSignature, *pPublicKey, "WRONG_MESSAGE");
	REQUIRE(wrongMessage == false);

	const std::unique_ptr<PublicKey> pPublicKey2 = Crypto::CalculatePublicKey(RandomNumberGenerator::GenerateRandom32());
	const bool differentPublicKey = Crypto::VerifyMessageSignature(*pSignature, *pPublicKey2, message);
	REQUIRE(differentPublicKey == false);
}