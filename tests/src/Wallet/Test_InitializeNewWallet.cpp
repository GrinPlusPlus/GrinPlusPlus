#include <catch.hpp>

#include <TestServer.h>
#include <TorProcessManager.h>

#include <Wallet/WalletManager.h>
#include <uuid.h>

TEST_CASE("Wallet Creation/Deletion")
{
	auto pServer = TestServer::CreateWithWallet();

	// Create wallet
	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	auto created_user = pServer->CreateUser(username, "Password1", UseTor::NO);

	// Validate seed words
	SecureString seedWords = created_user.wallet->GetSeedWords();
	REQUIRE(created_user.seed_words == seedWords);

	// Logout
	created_user.wallet->Logout();

	// Delete wallet
	pServer->GetWalletManager()->DeleteWallet(username, "Password1");
}

TEST_CASE("Wallet Words Length")
{
	auto pServer = TestServer::CreateWithWallet();

	// Create wallet
	std::vector<uint8_t> numWordsVec = { 12, 15, 18, 21, 24 };
	for (uint8_t numWords : numWordsVec)
	{
		const std::string username = uuids::to_string(uuids::uuid_system_generator()());
		auto created_user = pServer->CreateUser(username, "Password1", UseTor::NO, (SeedWordSize)numWords);

		// Validate seed words
		SecureString seedWords = created_user.wallet->GetSeedWords();
		REQUIRE(created_user.seed_words == seedWords);
		REQUIRE(StringUtil::Split((const std::string&)seedWords, " ").size() == numWords);

		// Logout
		created_user.wallet->Logout();

		// Delete wallet
		pServer->GetWalletManager()->DeleteWallet(username, "Password1");
	}
}