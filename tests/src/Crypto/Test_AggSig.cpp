#include <catch.hpp>

#include <secp256k1-zkp/secp256k1.h>
#include <Crypto/Crypto.h>
#include <Crypto/CSPRNG.h>

TEST_CASE("AggSig Interaction")
{
	Hash message = CSPRNG::GenerateRandom32();

	// Generate sender keypairs
	SecretKey secretKeySender = CSPRNG::GenerateRandom32();
	PublicKey publicKeySender = Crypto::CalculatePublicKey(secretKeySender);
	SecretKey secretNonceSender = Crypto::GenerateSecureNonce();
	PublicKey publicNonceSender = Crypto::CalculatePublicKey(secretNonceSender);

	// Generate receiver keypairs
	SecretKey secretKeyReceiver = CSPRNG::GenerateRandom32();
	PublicKey publicKeyReceiver = Crypto::CalculatePublicKey(secretKeyReceiver);
	SecretKey secretNonceReceiver = Crypto::GenerateSecureNonce();
	PublicKey publicNonceReceiver = Crypto::CalculatePublicKey(secretNonceReceiver);

	// Add pubKeys and pubNonces
	PublicKey sumPubKeys = Crypto::AddPublicKeys(std::vector<PublicKey>({ publicKeySender, publicKeyReceiver }));
	PublicKey sumPubNonces = Crypto::AddPublicKeys(std::vector<PublicKey>({ publicNonceSender, publicNonceReceiver }));

	// Generate partial signatures
	CompactSignature senderPartialSignature = Crypto::CalculatePartialSignature(secretKeySender, secretNonceSender, sumPubKeys, sumPubNonces, message);
	CompactSignature receiverPartialSignature = Crypto::CalculatePartialSignature(secretKeyReceiver, secretNonceReceiver, sumPubKeys, sumPubNonces, message);

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