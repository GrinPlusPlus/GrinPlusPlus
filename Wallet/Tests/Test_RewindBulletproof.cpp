#include <ThirdParty/Catch2/catch.hpp>

#include "../Keychain/KeyChain.h"
#include "../Keychain/KeyGenerator.h"
#include "../Wallet.h"

#include <Config/ConfigManager.h>

TEST_CASE("REWIND_BULLETPROOF_ORIGINAL")
{
	Config config = ConfigManager::LoadConfig(EEnvironmentType::MAINNET);

	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	const CBigInteger<32> masterSeed = RandomNumberGenerator::GenerateRandom32();
	const SecureVector masterSeedBytes(masterSeed.GetData().begin(), masterSeed.GetData().end());
	const uint64_t amount = 45;
	KeyChainPath keyId(std::vector<uint32_t>({ 1, 2, 3 }));

	KeyChain keyChain = KeyChain::FromSeed(config, masterSeedBytes);
	SecretKey blindingFactor = *keyChain.DerivePrivateKey(keyId, amount);
	Commitment commitment = *Crypto::CommitBlinded(amount, BlindingFactor(blindingFactor.GetBytes()));

	std::unique_ptr<RangeProof> pRangeProof = keyChain.GenerateRangeProof(keyId, amount, commitment, blindingFactor, EBulletproofType::ORIGINAL);
	std::unique_ptr<RewoundProof> pRewoundProof = keyChain.RewindRangeProof(commitment, *pRangeProof, EBulletproofType::ORIGINAL);

	REQUIRE(pRewoundProof != nullptr);
	REQUIRE(amount == pRewoundProof->GetAmount());
	REQUIRE(keyId.GetKeyIndices() == pRewoundProof->GetProofMessage().ToKeyIndices(EBulletproofType::ORIGINAL));
	REQUIRE(blindingFactor.GetBytes() == pRewoundProof->GetBlindingFactor()->GetBytes());
}

TEST_CASE("REWIND_BULLETPROOF_ENHANCED")
{
	Config config = ConfigManager::LoadConfig(EEnvironmentType::MAINNET);

	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	const CBigInteger<32> masterSeed = RandomNumberGenerator::GenerateRandom32();
	const SecureVector masterSeedBytes(masterSeed.GetData().begin(), masterSeed.GetData().end());
	const uint64_t amount = 45;
	KeyChainPath keyId(std::vector<uint32_t>({ 1, 2, 3 }));

	KeyChain keyChain = KeyChain::FromSeed(config, masterSeedBytes);
	SecretKey blindingFactor = *keyChain.DerivePrivateKey(keyId, amount);
	Commitment commitment = *Crypto::CommitBlinded(amount, BlindingFactor(blindingFactor.GetBytes()));

	std::unique_ptr<RangeProof> pRangeProof = keyChain.GenerateRangeProof(keyId, amount, commitment, blindingFactor, EBulletproofType::ENHANCED);
	std::unique_ptr<RewoundProof> pRewoundProof = keyChain.RewindRangeProof(commitment, *pRangeProof, EBulletproofType::ENHANCED);

	REQUIRE(pRewoundProof != nullptr);
	REQUIRE(amount == pRewoundProof->GetAmount());
	REQUIRE(keyId.GetKeyIndices() == pRewoundProof->GetProofMessage().ToKeyIndices(EBulletproofType::ENHANCED));
}