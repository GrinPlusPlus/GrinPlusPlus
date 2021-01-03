#include <catch.hpp>

#include <TestServer.h>
#include <TorProcessManager.h>

#include <Wallet/WalletManager.h>
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
		CreateWalletCriteria(username, "Password1", 24),
		TorProcessManager::GetProcess(0)
	);

	// Validate seed words
	SecureString seedWords = pWalletManager->GetWallet(response.GetToken()).Read()->GetSeedWords();
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
			CreateWalletCriteria(username, "Password1", numWords),
			TorProcessManager::GetProcess(0)
		);

		// Validate seed words
		auto wallet = pWalletManager->GetWallet(response.GetToken());
		SecureString seedWords = wallet.Read()->GetSeedWords();
		REQUIRE(response.GetSeed() == seedWords);
		REQUIRE(StringUtil::Split((const std::string&)response.GetSeed(), " ").size() == numWords);

		// Logout
		pWalletManager->Logout(response.GetToken());

		// Delete wallet
		pWalletManager->DeleteWallet(username, "Password1");
	}
}