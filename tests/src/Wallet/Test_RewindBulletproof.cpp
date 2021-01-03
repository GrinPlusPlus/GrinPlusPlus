#include <catch.hpp>

#include <Wallet/Keychain/KeyChain.h>
#include <Wallet/Keychain/KeyGenerator.h>
#include <Crypto/Crypto.h>
#include <uuid.h>

#include <Core/Config.h>

TEST_CASE("REWIND_BULLETPROOF_ORIGINAL")
{
	ConfigPtr pConfig = Config::Load(Environment::AUTOMATED_TESTING);

	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	const CBigInteger<32> masterSeed = CSPRNG::GenerateRandom32();
	const SecureVector masterSeedBytes(masterSeed.GetData().begin(), masterSeed.GetData().end());
	const uint64_t amount = 45;
	KeyChainPath keyId(std::vector<uint32_t>({ 1, 2, 3 }));

	KeyChain keyChain = KeyChain::FromSeed(*pConfig, masterSeedBytes);
	SecretKey blindingFactor = keyChain.DerivePrivateKey(keyId, amount);
	Commitment commitment = Crypto::CommitBlinded(amount, BlindingFactor(blindingFactor.GetBytes()));

	RangeProof rangeProof = keyChain.GenerateRangeProof(keyId, amount, commitment, blindingFactor, EBulletproofType::ORIGINAL);
	std::unique_ptr<RewoundProof> pRewoundProof = keyChain.RewindRangeProof(commitment, rangeProof, EBulletproofType::ORIGINAL);

	REQUIRE(pRewoundProof != nullptr);
	REQUIRE(amount == pRewoundProof->GetAmount());
	REQUIRE(keyId.GetKeyIndices() == pRewoundProof->GetProofMessage().ToKeyIndices(EBulletproofType::ORIGINAL));
	REQUIRE(blindingFactor.GetBytes() == pRewoundProof->GetBlindingFactor()->GetBytes());
}

TEST_CASE("REWIND_BULLETPROOF_ENHANCED")
{
	ConfigPtr pConfig = Config::Load(Environment::AUTOMATED_TESTING);

	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	const CBigInteger<32> masterSeed = CSPRNG::GenerateRandom32();
	const SecureVector masterSeedBytes(masterSeed.GetData().begin(), masterSeed.GetData().end());
	const uint64_t amount = 45;
	KeyChainPath keyId(std::vector<uint32_t>({ 1, 2, 3 }));

	KeyChain keyChain = KeyChain::FromSeed(*pConfig, masterSeedBytes);
	SecretKey blindingFactor = keyChain.DerivePrivateKey(keyId, amount);
	Commitment commitment = Crypto::CommitBlinded(amount, BlindingFactor(blindingFactor.GetBytes()));

	RangeProof rangeProof = keyChain.GenerateRangeProof(keyId, amount, commitment, blindingFactor, EBulletproofType::ENHANCED);
	std::unique_ptr<RewoundProof> pRewoundProof = keyChain.RewindRangeProof(commitment, rangeProof, EBulletproofType::ENHANCED);

	REQUIRE(pRewoundProof != nullptr);
	REQUIRE(amount == pRewoundProof->GetAmount());
	REQUIRE(keyId.GetKeyIndices() == pRewoundProof->GetProofMessage().ToKeyIndices(EBulletproofType::ENHANCED));
}