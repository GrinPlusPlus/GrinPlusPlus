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
	std::optional<std::pair<SecureString, SessionToken>> wallet = pWalletManager->InitializeNewWallet(
		CreateWalletCriteria(username, "Password1", 24)
	);
	REQUIRE(wallet.has_value());

	// Validate seed words
	SecureString seedWords = pWalletManager->GetSeedWords(wallet.value().second);
	REQUIRE(wallet.value().first == seedWords);

	// Logout
	pWalletManager->Logout(wallet.value().second);

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
		std::optional<std::pair<SecureString, SessionToken>> wallet = pWalletManager->InitializeNewWallet(
			CreateWalletCriteria(username, "Password1", numWords)
		);
		REQUIRE(wallet.has_value());

		// Validate seed words
		SecureString seedWords = pWalletManager->GetSeedWords(wallet.value().second);
		REQUIRE(wallet.value().first == seedWords);
		REQUIRE(StringUtil::Split((const std::string&)wallet.value().first, " ").size() == numWords);

		// Logout
		pWalletManager->Logout(wallet.value().second);

		// Delete wallet
		pWalletManager->DeleteWallet(username, "Password1");
	}
}