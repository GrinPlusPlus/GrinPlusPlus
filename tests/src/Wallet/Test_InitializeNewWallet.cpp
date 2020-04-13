#include <catch.hpp>

#include <TestServer.h>

#include <Wallet/WalletManager.h>
#include <Config/ConfigLoader.h>
#include <uuid.h>

TEST_CASE("Wallet Creation/Deletion")
{
	auto pServer = TestServer::Create();
	ConfigPtr pConfig = pServer->GetConfig();
	auto pNodeClient = pServer->GetNodeClient();
	IWalletManagerPtr pWalletManager = WalletAPI::CreateWalletManager(*pConfig, pNodeClient);

	// Create wallet
	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	auto response = pWalletManager->InitializeNewWallet(
		CreateWalletCriteria(username, "Password1", 24)
	);

	// Validate seed words
	SecureString seedWords = pWalletManager->GetSeedWords(response.GetToken());
	REQUIRE(response.GetSeed() == seedWords);

	// Logout
	pWalletManager->Logout(response.GetToken());

	// Delete wallet
	pWalletManager->DeleteWallet(username, "Password1");
}

TEST_CASE("Wallet Words Length")
{
	auto pServer = TestServer::Create();
	ConfigPtr pConfig = pServer->GetConfig();
	auto pNodeClient = pServer->GetNodeClient();
	IWalletManagerPtr pWalletManager = WalletAPI::CreateWalletManager(*pConfig, pNodeClient);

	// Create wallet
	std::vector<uint8_t> numWordsVec = { 12, 15, 18, 21, 24 };
	for (uint8_t numWords : numWordsVec)
	{
		const std::string username = uuids::to_string(uuids::uuid_system_generator()());
		auto response = pWalletManager->InitializeNewWallet(
			CreateWalletCriteria(username, "Password1", numWords)
		);

		// Validate seed words
		SecureString seedWords = pWalletManager->GetSeedWords(response.GetToken());
		REQUIRE(response.GetSeed() == seedWords);
		REQUIRE(StringUtil::Split((const std::string&)response.GetSeed(), " ").size() == numWords);

		// Logout
		pWalletManager->Logout(response.GetToken());

		// Delete wallet
		pWalletManager->DeleteWallet(username, "Password1");
	}
}